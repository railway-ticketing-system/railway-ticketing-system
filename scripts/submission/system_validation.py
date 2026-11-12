#!/usr/bin/env python3
"""Isolated real-process system validation. Python stdlib only; openssl required for TLS."""
import argparse
import concurrent.futures
import datetime as dt
import hashlib
import http.client
import json
import math
import os
from pathlib import Path
import platform
import secrets
import shutil
import socket
import sqlite3
import ssl
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def now():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def command(args):
    return subprocess.check_output(args, cwd=ROOT, text=True, timeout=15).strip()


class Runner:
    def __init__(self, binary, directory):
        self.binary, self.directory = binary, Path(directory)
        self.db = self.directory / 'validation.sqlite'
        self.process = None
        self.logs = []
        self.steps = []
        self.perf = {}
        self.password = secrets.token_urlsafe(20) + 'Aa1!'
        self.tokens = []
        self.tls = False
        self.context = None
        self.env = {k: v for k, v in os.environ.items() if not k.startswith('RTRS_')}
        self.env.update(RTRS_DB=str(self.db), RTRS_WEB_DIR=str(ROOT / 'web'))

    def redact(self, value):
        if isinstance(value, dict):
            return {k: ('[REDACTED]' if any(x in k.lower() for x in ('password', 'token', 'authorization')) else self.redact(v)) for k, v in value.items()}
        if isinstance(value, list):
            return [self.redact(v) for v in value]
        if isinstance(value, str):
            for secret in [self.password] + self.tokens:
                value = value.replace(secret, '[REDACTED]')
        return value

    def check(self, name, expected, actual, passed):
        self.steps.append(dict(name=name, timestamp=now(), expected=expected,
                               actual=self.redact(actual), status='PASS' if passed else 'FAIL'))
        print(f"{self.steps[-1]['status']}: {name}", flush=True)

    def request(self, method, path, body=None, token=None):
        headers = {'Content-Type': 'application/json'}
        if token:
            headers['Authorization'] = 'Bearer ' + token
        raw = body if isinstance(body, str) else json.dumps(body) if body is not None else None
        cls = http.client.HTTPSConnection if self.tls else http.client.HTTPConnection
        kwargs = {'context': self.context} if self.tls else {}
        conn = cls('localhost', self.port, timeout=10, **kwargs)
        start = time.perf_counter()
        try:
            conn.request(method, path, raw, headers)
            response = conn.getresponse()
            content = response.read()
            elapsed = time.perf_counter() - start
            try:
                data = json.loads(content)
            except (ValueError, UnicodeDecodeError):
                data = {'nonJsonBytes': len(content)}
            return response.status, data, elapsed, dict(response.getheaders())
        finally:
            conn.close()

    def api(self, name, method, path, expected=200, body=None, token=None):
        status, data, elapsed, headers = self.request(method, path, body, token)
        self.check(name, {'httpStatus': expected}, {'httpStatus': status, 'body': data,
                   'elapsedSeconds': elapsed}, status == expected)
        return data

    def stop(self):
        if self.process is not None:
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=8)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait(timeout=5)
            self.process = None
        for handle in self.logs:
            handle.flush()

    def start(self, tls=False):
        self.stop()
        self.tls = tls
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0))
            self.port = sock.getsockname()[1]
        env = dict(self.env, RTRS_PORT=str(self.port))
        if tls:
            env.update(RTRS_TLS_CERT=str(self.directory / 'cert.pem'), RTRS_TLS_KEY=str(self.directory / 'key.pem'))
            self.context = ssl.create_default_context(cafile=str(self.directory / 'cert.pem'))
        log = open(self.directory / f'server-{len(self.logs)}.log', 'w+', encoding='utf-8')
        self.logs.append(log)
        self.process = subprocess.Popen([str(self.binary)], cwd=self.directory, env=env,
                                        stdin=subprocess.DEVNULL, stdout=log, stderr=log,
                                        start_new_session=True)
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            if self.process.poll() is not None:
                raise RuntimeError('Server exited during startup; exit code ' + str(self.process.returncode))
            try:
                if self.request('GET', '/api/health')[:2] == (200, {'status': 'ok'}):
                    self.check('Start HTTPS server' if tls else 'Start HTTP server', 'Healthy isolated process within 15 seconds',
                               {'port': self.port, 'pid': self.process.pid, 'tls': tls}, True)
                    return
            except (OSError, http.client.HTTPException):
                pass
            time.sleep(.05)
        raise RuntimeError('Server readiness timeout')

    def login(self, email):
        data = self.api('Login ' + email, 'POST', '/api/auth/login', body={'email': email, 'password': self.password})
        token = data['token']
        self.tokens.append(token)
        return token

    def run(self):
        from urllib.parse import urlencode
        self.start()
        date = (dt.date.today() + dt.timedelta(days=3)).isoformat()
        self.travel_date = date
        search = '/api/trains/search?' + urlencode({'source': 'Pune Jn', 'destination': 'Mumbai CSMT', 'date': date})
        for email in ('owner@example.test', 'other@example.test', 'admin@example.test'):
            data = self.api('Register ' + email, 'POST', '/api/auth/register', 201,
                            {'name': 'System Passenger', 'email': email, 'phone': '9876543210', 'password': self.password, 'role': 'ADMIN'})
            self.check('Registration cannot assign admin: ' + email, 'PASSENGER', data.get('user', {}).get('role'), data.get('user', {}).get('role') == 'PASSENGER')
        owner, other, candidate = [self.login(e) for e in ('owner@example.test', 'other@example.test', 'admin@example.test')]
        self.api('Anonymous profile denied', 'GET', '/api/users/me', 401)
        self.api('Profile update', 'PUT', '/api/users/me', body={'name': 'Updated Passenger', 'phone': '9123456780', 'userId': 2, 'role': 'ADMIN'}, token=owner)
        profile = self.api('Profile read', 'GET', '/api/users/me', token=owner)['user']
        self.check('Profile update and role protection', 'Updated Passenger, PASSENGER, owner email', profile,
                   profile['name'] == 'Updated Passenger' and profile['role'] == 'PASSENGER' and profile['email'] == 'owner@example.test')
        data = self.api('Future Pune to Mumbai search', 'GET', search)
        self.check('Seeded route results', 'Deccan and Shatabdi trains', data, {'11007', '12027'} <= {t['number'] for t in data.get('trains', [])})
        train = next(t for t in data['trains'] if t['number'] == '11007')
        base = '/api/trains/' + str(train['id'])
        schedule = self.api('Schedule', 'GET', base + '/schedule')
        self.check('Ordered route stops', ['Pune Jn', 'Lonavala', 'Mumbai CSMT'], schedule['stops'], [s['station'] for s in schedule['stops']] == ['Pune Jn', 'Lonavala', 'Mumbai CSMT'])
        availability = base + '/availability?date=' + date
        def seats():
            return next(c for c in self.api('Seat inventory', 'GET', availability)['classes'] if c['code'] == 'SL')
        initial = seats()
        fares = self.api('Class fares', 'GET', base + '/fares')['classes']
        self.check('Fare consistency', 'SL 190 and same availability fare', fares, next(c['fare'] for c in fares if c['code'] == 'SL') == initial['fare'] == 190)
        booking_body = {'trainId': train['id'], 'date': date, 'class': 'SL', 'passengers': [{'name': 'Asha Rao', 'age': 30, 'gender': 'F'}, {'name': 'Ravi Kumar', 'age': 35, 'gender': 'M'}]}
        preview = self.api('Booking preview', 'POST', '/api/bookings/preview', body=booking_body, token=owner)['preview']
        self.check('Preview fare arithmetic', 380, preview, preview['totalFare'] == 380 and preview['passengerCount'] == 2)
        self.check('Preview does not reserve', initial['availableSeats'], seats()['availableSeats'], seats()['availableSeats'] == initial['availableSeats'])
        booking = self.api('Create booking', 'POST', '/api/bookings', 201, booking_body, owner)['booking']
        pnrpath = '/api/bookings/' + booking['pnr']
        idpath = '/api/bookings/' + str(booking['id'])
        self.check('Confirmed ticket and assigned unique seats', 'CONFIRMED, numeric PNR, 2 unique seats, fare 380', booking,
                   booking['status'] == 'CONFIRMED' and booking['pnr'].isdigit() and len({p['seatNumber'] for p in booking['passengers']}) == 2 and booking['totalFare'] == 380)
        self.check('Seat decrement', initial['availableSeats'] - 2, seats()['availableSeats'], seats()['availableSeats'] == initial['availableSeats'] - 2)
        ticket = self.api('Owner ticket', 'GET', pnrpath, token=owner)['booking']
        self.check('Ticket matches confirmed booking', booking, ticket, ticket == booking)
        history = self.api('Owner history', 'GET', '/api/bookings', token=owner)['bookings']
        self.check('History contains booking', booking['pnr'], history, any(b['pnr'] == booking['pnr'] for b in history))
        for suffix, method in (('', 'GET'), ('/cancellation', 'GET'), ('/cancel', 'POST'), ('/cancel', 'PUT')):
            self.api('Cross-user denied ' + method + suffix, method, pnrpath if not suffix else idpath + suffix, 404, token=other)
        otherhistory = self.api('Other user history', 'GET', '/api/bookings', token=other)['bookings']
        self.check('History isolation', [], otherhistory, otherhistory == [])
        quote = self.api('Refund quote', 'GET', idpath + '/cancellation', token=owner)['cancellation']
        self.check('Default cancellation policy', 'eligible, cutoff 24h, 90%, floor(380*.9)=342', quote, quote['eligible'] and quote['cutoffHours'] == 24 and quote['refundPercent'] == 90 and quote['refundAmount'] == 342)
        cancelled = self.api('Cancel booking', 'POST', idpath + '/cancel', token=owner)['booking']
        self.check('Refund recorded and cancellation timestamp', 'CANCELLED, refund 342, timestamp', cancelled, cancelled['status'] == 'CANCELLED' and cancelled['refundAmount'] == 342 and bool(cancelled['cancelledAt']))
        self.api('Repeat cancellation rejected', 'POST', idpath + '/cancel', 409, token=owner)
        restored = seats()['availableSeats']
        self.check('Seats restored exactly once', initial['availableSeats'], restored, restored == initial['availableSeats'])
        for path in ('/api/admin/me', '/api/admin/trains', '/api/admin/audit'):
            self.api('Passenger admin denial ' + path, 'GET', path, 403, token=other)
            self.api('Anonymous admin denial ' + path, 'GET', path, 401)
        self.api('Passenger admin write denied', 'POST', '/api/admin/trains', 403, {}, other)
        self.stop()
        result = subprocess.run([str(self.binary), '--promote-admin', 'admin@example.test'], env=self.env, cwd=self.directory, capture_output=True, timeout=15)
        self.check('Stopped-server admin promotion CLI', 'exit 0', {'returncode': result.returncode}, result.returncode == 0)
        self.start()
        self.api('Promotion revokes old session', 'GET', '/api/users/me', 401, token=candidate)
        admin = self.login('admin@example.test')
        role = self.api('Admin role', 'GET', '/api/admin/me', token=admin)['user']['role']
        self.check('Promoted role', 'ADMIN', role, role == 'ADMIN')
        managed = {'number': '94999', 'name': 'Validation Express', 'stops': [{'station': 'Cedar Jn', 'arrival': '', 'departure': '08:00', 'dayOffset': 0}, {'station': 'Maple Jn', 'arrival': '10:00', 'departure': '', 'dayOffset': 0}], 'classes': [{'code': 'SL', 'name': 'Sleeper', 'fare': 250, 'totalSeats': 50}]}
        created = self.api('Admin create train', 'POST', '/api/admin/trains', 201, managed, admin)['train']
        managedpath = '/api/admin/trains/' + str(created['id'])
        managed['name'] = 'Updated Validation Express'
        managed['classes'][0]['fare'] = 275
        updated = self.api('Admin update train', 'PUT', managedpath, body=managed, token=admin)['train']
        self.check('Admin update reflected', managed['name'], updated, updated['name'] == managed['name'] and updated['classes'][0]['fare'] == 275)
        managedsearch = '/api/trains/search?' + urlencode({'source': 'Cedar Jn', 'destination': 'Maple Jn', 'date': date})
        visible = self.api('Created train public visibility', 'GET', managedsearch)['trains']
        self.check('Managed train searchable', created['id'], visible, any(t['id'] == created['id'] for t in visible))
        inactive = self.api('Admin deactivate train', 'PUT', managedpath + '/deactivate', token=admin)['train']
        self.check('Train inactive', False, inactive['active'], inactive['active'] is False)
        hidden = self.api('Inactive train excluded from search', 'GET', managedsearch)['trains']
        self.check('No inactive results', [], hidden, hidden == [])
        self.api('Inactive schedule unavailable', 'GET', '/api/trains/' + str(created['id']) + '/schedule', 404)
        events = self.api('Admin audit', 'GET', '/api/admin/audit?limit=500', token=admin)['events']
        matching = [e for e in events if e['bookingId'] == booking['id']]
        self.check('Booking audit correlates actor, booking, refund', 'creation + cancellation with correct actor/refund', matching,
                   {e['event'] for e in matching} >= {'BOOKING_CREATED', 'BOOKING_CANCELLED'} and all(e['userId'] == profile['id'] for e in matching) and any(e['event'] == 'BOOKING_CANCELLED' and e['refundAmount'] == 342 for e in matching))
        # Errors are exercised independently of success flows; never store request secrets.
        for name, method, path, status, body, token in [
            ('Malformed JSON', 'POST', '/api/auth/register', 400, '{broken', None),
            ('Invalid booking passengers', 'POST', '/api/bookings', 400, dict(booking_body, passengers=[]), owner),
            ('Invalid date', 'GET', base + '/availability?date=bad', 400, None, None),
            ('Unknown endpoint', 'GET', '/api/no-such-resource', 404, None, None),
            ('Invalid audit limit', 'GET', '/api/admin/audit?limit=501', 400, None, admin),
            ('Anonymous booking', 'POST', '/api/bookings', 401, booking_body, None)]:
            error = self.api(name, method, path, status, body, token)
            serialized = json.dumps(error).lower()
            self.check(name + ' safe error', 'JSON error without implementation details', error,
                       isinstance(error.get('error'), str) and not any(s in serialized for s in ('sqlite', 'traceback', 'select ', 'std::', str(self.directory))))
        unknown = self.api('Unknown account login', 'POST', '/api/auth/login', 401, {'email': 'unknown@example.test', 'password': self.password})
        wrong = self.api('Wrong password login', 'POST', '/api/auth/login', 401, {'email': 'owner@example.test', 'password': 'Incorrect1!'})
        self.check('Login does not enumerate accounts', unknown, wrong, unknown == wrong)
        # Four clients, twenty requests per operation; all latencies include connect + complete body.
        for label in ('search', 'booking'):
            def work(_):
                return self.request('GET', search) if label == 'search' else self.request('POST', '/api/bookings', booking_body, owner)
            started = time.perf_counter()
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                results = list(pool.map(work, range(20)))
            samples = [r[2] for r in results]
            expected = 200 if label == 'search' else 201
            valid = all(r[0] == expected and (r[1].get('count', 0) >= 2 if label == 'search' else r[1].get('booking', {}).get('status') == 'CONFIRMED') for r in results)
            metric = dict(requests=20, concurrency=4, latencySeconds=samples, p90Seconds=sorted(samples)[math.ceil(.9 * len(samples)) - 1], maxSeconds=max(samples), wallSeconds=time.perf_counter() - started, httpStatuses=[r[0] for r in results], percentileMethod='nearest rank ceil(0.9*n)', thresholdSeconds=3)
            self.perf[label] = metric
            self.check(label + ' small-load p90', '20 successful operations, 4 clients, p90 <3s', metric, valid and metric['p90Seconds'] < 3)
            if label == 'booking':
                active = [r[1]['booking'] for r in results if r[0] == 201]
                self.check('Concurrent booking unique PNRs and seats', '20 unique PNRs and 40 unique seats', {'pnrs': len({b['pnr'] for b in active}), 'seats': len({p['seatNumber'] for b in active for p in b['passengers']})}, len({b['pnr'] for b in active}) == 20 and len({p['seatNumber'] for b in active for p in b['passengers']}) == 40)
        before_restart = self.api('Pre-restart history', 'GET', '/api/bookings', token=owner)['bookings']
        self.stop()
        self.start()
        owner = self.login('owner@example.test')
        persisted = self.api('Persisted profile', 'GET', '/api/users/me', token=owner)['user']
        history2 = self.api('Persisted bookings', 'GET', '/api/bookings', token=owner)['bookings']
        self.check('Restart persistence exact history/profile', 'Same history and updated profile', {'bookings': len(history2), 'profile': persisted}, history2 == before_restart and persisted == profile)
        self.check('Restart inventory persists', initial['availableSeats'] - 40, seats()['availableSeats'], seats()['availableSeats'] == initial['availableSeats'] - 40)
        admin = self.login('admin@example.test')
        persisted_train = self.api('Persisted admin train', 'GET', managedpath, token=admin)['train']
        persisted_events = self.api('Persisted audit', 'GET', '/api/admin/audit?limit=500', token=admin)['events']
        self.check('Admin state and audit survive restart', 'Inactive edited train and previous audit events', {'train': persisted_train, 'events': len(persisted_events)}, persisted_train == inactive and all(e in persisted_events for e in events))
        # Abrupt termination after committed operations tests process-crash recovery.
        self.process.kill()
        crash_code = self.process.wait(timeout=5)
        self.process = None
        self.check('Abrupt process stop', 'Nonzero killed-process return code', {'returncode': crash_code}, crash_code != 0)
        self.start()
        owner = self.login('owner@example.test')
        recovered = self.api('Crash-recovered history', 'GET', '/api/bookings', token=owner)['bookings']
        self.check('Committed bookings survive abrupt stop', before_restart, recovered, recovered == before_restart)
        self.check('Crash-recovered inventory', initial['availableSeats'] - 40, seats()['availableSeats'], seats()['availableSeats'] == initial['availableSeats'] - 40)
        with sqlite3.connect('file:' + str(self.db) + '?mode=ro', uri=True) as connection:
            integrity = connection.execute('PRAGMA integrity_check').fetchall()
        self.check('Recovered SQLite integrity', [['ok']], [list(r) for r in integrity], integrity == [('ok',)])
        self.api('Logout', 'POST', '/api/auth/logout', token=owner)
        self.api('Logged-out session denied', 'GET', '/api/users/me', 401, token=owner)
        self.stop()
        openssl = shutil.which('openssl')
        if not openssl:
            self.check('Real TLS', 'openssl available for temporary self-signed certificate', 'openssl missing', False)
            return
        result = subprocess.run([openssl, 'req', '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', str(self.directory / 'key.pem'), '-out', str(self.directory / 'cert.pem'), '-days', '1', '-subj', '/CN=localhost', '-addext', 'subjectAltName=DNS:localhost,IP:127.0.0.1'], capture_output=True, timeout=30)
        self.check('Generate temporary TLS certificate', 'openssl exit 0', {'returncode': result.returncode}, result.returncode == 0)
        if result.returncode:
            return
        os.chmod(self.directory / 'key.pem', 0o600)
        self.start(tls=True)
        with socket.create_connection(('localhost', self.port), timeout=5) as sock:
            with self.context.wrap_socket(sock, server_hostname='localhost') as secure:
                evidence = {'protocol': secure.version(), 'cipher': secure.cipher(), 'peerCertificateSha256': hashlib.sha256(secure.getpeercert(binary_form=True)).hexdigest(), 'hostnameVerified': True, 'certificateVerification': 'CERT_REQUIRED using temporary CA certificate'}
        self.check('Real verified TLS handshake', 'TLSv1.2 or TLSv1.3 with hostname verification', evidence, evidence['protocol'] in ('TLSv1.2', 'TLSv1.3'))
        status, body, _, headers = self.request('GET', '/api/health')
        self.check('HTTPS health and security headers', '200 healthy, HSTS, nosniff, DENY', {'status': status, 'body': body, 'headers': headers}, status == 200 and body == {'status': 'ok'} and headers.get('Strict-Transport-Security') == 'max-age=31536000' and headers.get('X-Content-Type-Options') == 'nosniff' and headers.get('X-Frame-Options') == 'DENY')
        secure_token = self.login('owner@example.test')
        self.api('Authenticated profile over real TLS', 'GET', '/api/users/me', token=secure_token)

    def cleanup(self):
        self.stop()
        logtext = ''
        for handle in self.logs:
            handle.seek(0)
            logtext += handle.read()
            handle.close()
        leaked = any(s in logtext for s in [self.password] + self.tokens)
        self.check('Server logs contain no generated credentials', 'No passwords or bearer tokens in subprocess logs', {'credentialLeakDetected': leaked, 'logBytes': len(logtext.encode()), 'processesStopped': self.process is None}, not leaked)


def write_markdown(report, destination):
    """Render a compact review summary without mutating the raw evidence."""
    environment = dict(report['environment'])
    # Older evidence used the ambiguous key 'sqlite'; preserve that JSON unchanged.
    if 'sqlite' in environment:
        environment['pythonSqliteVersion'] = environment.pop('sqlite')
    lines = ['# System Validation Report', '', f"Outcome: **{report['status']}** — {report['summary']['PASS']} passed, {report['summary']['FAIL']} failed.", '', f"Run: {report['startedAt']} to {report['finishedAt']}", f"Source SHA: `{report['sourceSha']}` (finish `{report['sourceShaAtFinish']}`)", f"Binary: `{report['binary']}`", f"Binary SHA-256: `{report.get('binarySha256', 'unavailable')}`", f"Travel date: {report.get('travelDate', 'not reached')} (system local date +3 days).", '', '## Environment', '', '```json', json.dumps(environment, indent=2), '```', '', 'The Python SQLite version above describes the runner’s `sqlite3` module, not the server. The application bundles SQLite 3.46.1; this is not a runtime server-version measurement.', '', '## Method and scope', '']
    lines += [f'- **{k}:** {v}' for k, v in report['methodology'].items() if k != 'limits']
    lines += ['', '## Measured performance', '', '| Operation | Requests | Clients | p90 (s) | Max (s) | Threshold |', '|---|---:|---:|---:|---:|---|']
    for label, metric in report.get('performance', {}).items():
        lines.append(f"| {label} | {metric['requests']} | {metric['concurrency']} | {metric['p90Seconds']:.6f} | {metric['maxSeconds']:.6f} | <3 s |")
    lines += ['', 'Detailed expected/actual bodies, raw latency samples and per-step timestamps are preserved in [raw JSON evidence](system_validation.json). Check IDs below identify the corresponding 1-based entries in its `steps` array.', '', '## Failures and limitations', '']
    failures = [s for s in report['steps'] if s['status'] == 'FAIL']
    lines += [f"- {s['name']}: FAIL; see raw JSON for expected/actual evidence." for s in failures] or ['No failed assertions observed in executed checks.']
    if report.get('incomplete'):
        lines.append('**Run incomplete: stages after the runtime/prerequisite failure were not executed; this is not a complete system pass.**')
    lines += ['- ' + limitation for limitation in report['methodology']['limits']]
    lines += ['', '## Check results', '', '| Check ID | Name | Status |', '|---|---|---|']
    for i, step in enumerate(report['steps'], 1):
        name = step['name'].replace('|', '\\|').replace('\n', ' ')
        lines.append(f"| SV-{i:03d} | {name} | {step['status']} |")
    destination.write_text('\n'.join(lines) + '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, help='Server executable (relative to current working directory)')
    parser.add_argument('--report-only', action='store_true', help='Regenerate Markdown from existing JSON without launching the server or changing evidence')
    args = parser.parse_args()
    if args.report_only:
        out = ROOT / 'docs/reports'
        report = json.loads((out / 'system_validation.json').read_text())
        write_markdown(report, out / 'System_Validation_Report.md')
        print(f"Markdown regenerated from existing JSON: {out / 'System_Validation_Report.md'}")
        return 0
    binary = args.binary or next((p for p in (ROOT / 'build-submission/rtrs_server', ROOT / 'build/rtrs_server') if p.is_file()), ROOT / 'build-submission/rtrs_server')
    binary = binary.resolve()
    started = now()
    report = dict(schemaVersion=1, startedAt=started, sourceSha=command(['git', 'rev-parse', 'HEAD']),
                  sourceStatus=command(['git', 'status', '--short']), binary=str(binary),
                  environment={'platform': platform.platform(), 'python': sys.version, 'pythonSqliteVersion': sqlite3.sqlite_version, 'openssl': ssl.OPENSSL_VERSION, 'cpuCount': os.cpu_count(), 'timezone': list(time.tzname)},
                  methodology={'isolation': 'Fresh TemporaryDirectory under /tmp, dedicated SQLite DB; inherited RTRS_* settings removed; ephemeral ports; no production DB', 'transport': 'Actual server subprocess and localhost HTTP/HTTPS sockets; no mocks', 'load': '20 search + 20 two-passenger bookings, separately, four concurrent clients, fresh connection per request, nearest-rank p90 <3 seconds each; connection through full response latency', 'recovery': 'Controlled restart and abrupt process kill/relaunch after committed writes, same database plus read-only SQLite integrity check; not power-loss or interrupted-transaction durability', 'credentials': 'Random runtime password; request bodies and auth headers never logged by runner; response secrets redacted', 'limits': ['Small local load only; no sustained stress or internet latency conclusions', 'TLS uses a temporary self-signed trust anchor; does not validate production certificates', 'Admin audit API records booking events; train changes verified via read-back and restart', 'No UI, screenshot, coverage or unit-test results claimed by this independent runner']})
    runner = None
    try:
        if not binary.is_file() or not os.access(binary, os.X_OK):
            raise RuntimeError('Executable not available: ' + str(binary))
        report['binarySha256'] = hashlib.sha256(binary.read_bytes()).hexdigest()
        report['binaryMtimeUtc'] = dt.datetime.fromtimestamp(binary.stat().st_mtime, dt.timezone.utc).isoformat()
        with tempfile.TemporaryDirectory(prefix='rtrs-system-validation-', dir='/tmp') as directory:
            runner = Runner(binary, directory)
            try:
                runner.run()
            except Exception as exc:
                runner.check('Runner completed all planned stages', 'No unhandled runtime failure', {'exceptionType': type(exc).__name__, 'message': str(exc)}, False)
                report['incomplete'] = True
            finally:
                runner.cleanup()
            report.update(steps=runner.steps, performance=runner.perf, travelDate=getattr(runner, 'travel_date', None), temporaryDataRemoved=True)
    except Exception as exc:
        report.update(steps=[dict(name='Executable prerequisite', timestamp=now(), expected='Executable server', actual=str(exc), status='FAIL')], incomplete=True)
    report['finishedAt'] = now()
    report['sourceShaAtFinish'] = command(['git', 'rev-parse', 'HEAD'])
    report['summary'] = {s: sum(step['status'] == s for step in report['steps']) for s in ('PASS', 'FAIL')}
    report['status'] = 'PASS' if report['summary']['FAIL'] == 0 and not report.get('incomplete') else 'FAIL'
    if runner:
        report = runner.redact(report)
    out = ROOT / 'docs/reports'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'system_validation.json').write_text(json.dumps(report, indent=2) + '\n')
    write_markdown(report, out / 'System_Validation_Report.md')
    print(f"Reports: {out / 'system_validation.json'} and {out / 'System_Validation_Report.md'}")
    return 0 if report['status'] == 'PASS' else 1


if __name__ == '__main__':
    sys.exit(main())
