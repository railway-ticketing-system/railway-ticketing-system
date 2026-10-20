'use strict';
const app = document.getElementById('app');

const session = {
  get token() { try { return sessionStorage.getItem('rtrs.token'); } catch (_) { return null; } },
  set token(v) { try { v ? sessionStorage.setItem('rtrs.token', v) : sessionStorage.removeItem('rtrs.token'); } catch (_) { /* storage blocked */ } },
};

async function api(method, path, body) {
  const headers = { 'Content-Type': 'application/json' };
  if (session.token) headers['Authorization'] = 'Bearer ' + session.token;
  const res = await fetch(path, {
    method,
    headers,
    body: body ? JSON.stringify(body) : undefined,
  });
  let data = {};
  try { data = await res.json(); } catch (_) { /* empty body */ }
  return { ok: res.ok, status: res.status, data };
}

function el(html) {
  const t = document.createElement('template');
  t.innerHTML = html.trim();
  return t.content.firstChild;
}

function showFieldErrors(form, fields) {
  form.querySelectorAll('input').forEach(i => {
    const msg = fields && fields[i.name];
    i.classList.toggle('invalid', !!msg);
    form.querySelector(`[data-err="${i.name}"]`).textContent = msg || '';
  });
}

function registerView() {
  const view = el(`
    <div class="card">
      <h2>Create account</h2>
      <form id="reg" novalidate>
        <label>Full name<input name="name" autocomplete="name"><span class="err" data-err="name"></span></label>
        <label>Email<input name="email" type="email" autocomplete="email"><span class="err" data-err="email"></span></label>
        <label>Phone<input name="phone" inputmode="numeric" maxlength="10" autocomplete="tel"><span class="err" data-err="phone"></span></label>
        <label>Password<input name="password" type="password" autocomplete="new-password"><span class="err" data-err="password"></span></label>
        <button type="submit">Register</button>
        <div id="status"></div>
      </form>
      <p class="muted">Already registered? <a id="to-login">Log in</a></p>
    </div>`);
  view.querySelector('#to-login').addEventListener('click', () => show('login'));
  const form = view.querySelector('form');
  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const status = form.querySelector('#status');
    const btn = form.querySelector('button');
    btn.disabled = true;
    const payload = Object.fromEntries(new FormData(form));
    const r = await api('POST', '/api/auth/register', payload);
    btn.disabled = false;
    showFieldErrors(form, r.data.fields);
    if (r.ok) {
      form.reset();
      status.innerHTML = '<div class="msg ok">Account created. You can now log in.</div>';
    } else {
      status.innerHTML = '';
      const m = el('<div class="msg bad"></div>');
      m.textContent = r.data.error || 'Registration failed.';
      status.appendChild(m);
    }
  });
  return view;
}

function loginView() {
  const view = el(`
    <div class="card">
      <h2>Log in</h2>
      <form novalidate>
        <label>Email<input name="email" type="email" autocomplete="username"><span class="err" data-err="email"></span></label>
        <label>Password<input name="password" type="password" autocomplete="current-password"><span class="err" data-err="password"></span></label>
        <button type="submit">Log in</button>
        <div id="status"></div>
      </form>
      <p class="muted">New here? <a id="to-register">Create an account</a></p>
    </div>`);
  view.querySelector('#to-register').addEventListener('click', () => show('register'));
  const form = view.querySelector('form');
  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const btn = form.querySelector('button');
    const status = form.querySelector('#status');
    btn.disabled = true;
    const r = await api('POST', '/api/auth/login', Object.fromEntries(new FormData(form)));
    btn.disabled = false;
    if (r.ok) {
      session.token = r.data.token;
      show('home', r.data.user);
    } else {
      const m = el('<div class="msg bad"></div>');
      m.textContent = r.data.error || 'Login failed.';
      status.replaceChildren(m);
    }
  });
  return view;
}

function profileView(user) {
  const view = el(`
    <div class="card">
      <h2>My profile</h2>
      <form id="profile" novalidate>
        <label>Email<input name="email" disabled></label>
        <label>Full name<input name="name"><span class="err" data-err="name"></span></label>
        <label>Phone<input name="phone" inputmode="numeric" maxlength="10"><span class="err" data-err="phone"></span></label>
        <button type="submit">Save changes</button>
        <div id="status"></div>
      </form>
      <h2 style="margin-top:24px">Change password</h2>
      <form id="pw" novalidate>
        <label>Current password<input name="currentPassword" type="password" autocomplete="current-password"><span class="err" data-err="currentPassword"></span></label>
        <label>New password<input name="newPassword" type="password" autocomplete="new-password"><span class="err" data-err="newPassword"></span></label>
        <button type="submit">Change password</button>
        <div id="status"></div>
      </form>
      <p class="muted"><a id="back">Back</a></p>
    </div>`);
  const pf = view.querySelector('#profile');
  pf.email.value = user.email;
  pf.name.value = user.name;
  pf.phone.value = user.phone;
  view.querySelector('#back').addEventListener('click', () => show('home', user));

  function notify(form, ok, text) {
    const m = el(`<div class="msg ${ok ? 'ok' : 'bad'}"></div>`);
    m.textContent = text;
    form.querySelector('#status').replaceChildren(m);
  }
  pf.addEventListener('submit', async (e) => {
    e.preventDefault();
    const r = await api('PUT', '/api/users/me', { name: pf.name.value, phone: pf.phone.value });
    showFieldErrors(pf, r.data.fields);
    if (r.ok) { user = r.data.user; notify(pf, true, 'Profile updated.'); }
    else if (r.status === 401) show('login');
    else notify(pf, false, r.data.error || 'Update failed.');
  });
  const pw = view.querySelector('#pw');
  pw.addEventListener('submit', async (e) => {
    e.preventDefault();
    const r = await api('PUT', '/api/users/me/password', Object.fromEntries(new FormData(pw)));
    showFieldErrors(pw, r.data.fields);
    if (r.ok) { pw.reset(); notify(pw, true, 'Password changed. Other devices were logged out.'); }
    else if (r.status === 401) show('login');
    else notify(pw, false, r.data.error || 'Password was not changed.');
  });
  return view;
}

function homeView(user) {
  const view = el('<div class="card"><h2></h2><p class="muted">You are logged in.</p><button id="search-btn">Search trains</button><button id="profile-btn">My profile</button><button id="logout">Log out</button><div id="status"></div></div>');
  view.querySelector('h2').textContent = 'Welcome, ' + user.name;
  view.querySelector('#search-btn').addEventListener('click', () => show('search', user));
  view.querySelector('#profile-btn').addEventListener('click', () => show('profile', user));
  view.querySelector('#logout').addEventListener('click', async () => {
    await api('POST', '/api/auth/logout');
    session.token = null;  // clear locally even if the server already dropped the session
    show('login');
  });
  return view;
}

function formatDuration(minutes) {
  const h = Math.floor(minutes / 60);
  const m = minutes % 60;
  return h + 'h ' + String(m).padStart(2, '0') + 'm';
}

function searchView(user) {
  const today = new Date().toISOString().slice(0, 10);
  const view = el(`
    <div class="card">
      <h2>Search trains</h2>
      <form id="search" novalidate>
        <label>From<input name="source" placeholder="Pune Jn"><span class="err" data-err="source"></span></label>
        <label>To<input name="destination" placeholder="Mumbai CSMT"><span class="err" data-err="destination"></span></label>
        <label>Travel date<input name="date" type="date" value="${today}"><span class="err" data-err="date"></span></label>
        <button type="submit">Search</button>
        <div id="status"></div>
      </form>
      <div id="results"></div>
      <p class="muted"><a id="back">Back</a></p>
    </div>`);
  view.querySelector('#back').addEventListener('click', () => show('home', user));

  const form = view.querySelector('#search');
  const results = view.querySelector('#results');
  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    form.querySelector('#status').replaceChildren();
    results.replaceChildren();
    const params = new URLSearchParams(Object.fromEntries(new FormData(form)));
    const r = await api('GET', '/api/trains/search?' + params.toString());
    showFieldErrors(form, r.data.fields);
    if (!r.ok) {
      const message = el('<div class="msg bad"></div>');
      message.textContent = r.data.error || 'Search failed.';
      form.querySelector('#status').replaceChildren(message);
      return;
    }
    if (!r.data.trains.length) {
      results.appendChild(el('<p class="muted">No trains found for this route and date.</p>'));
      return;
    }
    const list = el('<div class="results"></div>');
    for (const train of r.data.trains) {
      const item = el(`
        <div class="result">
          <div class="result-head"><strong></strong><span class="num"></span></div>
          <div class="times"><span class="dep"></span><span class="arrow">to</span><span class="arr"></span></div>
          <div class="meta"><span class="dur"></span><span class="fare"></span></div>
        </div>`);
      item.querySelector('strong').textContent = train.name;
      item.querySelector('.num').textContent = '#' + train.number;
      item.querySelector('.dep').textContent = train.departure + ' ' + train.source;
      item.querySelector('.arr').textContent = train.arrival + ' ' + train.destination;
      item.querySelector('.dur').textContent = formatDuration(train.durationMinutes);
      item.querySelector('.fare').textContent = 'from Rs ' + train.fromFare;
      item.classList.add('clickable');
      item.addEventListener('click', () => showSchedule(train, r.data.date, user));
      list.appendChild(item);
    }
    results.appendChild(list);
  });
  return view;
}

function showSchedule(train, date, user) {
  const view = el(`
    <div class="card">
      <h2></h2>
      <p class="muted" id="route"></p>
      <h3>Schedule</h3>
      <div id="schedule"><p class="muted">Loading schedule...</p></div>
      <h3>Seat availability</h3>
      <div id="availability"><p class="muted">Loading availability...</p></div>
      <p class="muted"><a id="back">Back to search</a></p>
    </div>`);
  view.querySelector('h2').textContent = train.name + ' #' + train.number;
  view.querySelector('#route').textContent = train.source + ' to ' + train.destination + ' on ' + date;
  view.querySelector('#back').addEventListener('click', () => show('search', user));
  app.replaceChildren(view);

  api('GET', '/api/trains/' + train.id + '/schedule').then((r) => {
    const box = view.querySelector('#schedule');
    if (!r.ok) {
      const message = el('<div class="msg bad"></div>');
      message.textContent = r.data.error || 'Could not load the schedule.';
      box.replaceChildren(message);
      return;
    }
    const list = el('<div class="schedule"></div>');
    for (const stop of r.data.stops) {
      const row = el('<div class="stop"><span class="station"></span><span class="time"></span></div>');
      row.querySelector('.station').textContent = stop.station;
      const times = [];
      if (stop.arrival) times.push('arr ' + stop.arrival);
      if (stop.departure) times.push('dep ' + stop.departure);
      if (stop.dayOffset) times.push('+' + stop.dayOffset + 'd');
      row.querySelector('.time').textContent = times.join(' / ');
      list.appendChild(row);
    }
    box.replaceChildren(list);
  });

  api('GET', '/api/trains/' + train.id + '/availability?date=' + encodeURIComponent(date)).then((r) => {
    const box = view.querySelector('#availability');
    if (!r.ok) {
      const message = el('<div class="msg bad"></div>');
      message.textContent = r.data.error || 'Could not load seat availability.';
      box.replaceChildren(message);
      return;
    }
    const list = el('<div class="classes"></div>');
    for (const cls of r.data.classes) {
      const row = el('<div class="class-row"><span class="cname"></span><span class="avail"></span><span class="cfare"></span></div>');
      row.querySelector('.cname').textContent = cls.name + ' (' + cls.code + ')';
      row.querySelector('.avail').textContent = cls.availableSeats + ' / ' + cls.totalSeats + ' available';
      row.querySelector('.cfare').textContent = 'Rs ' + cls.fare;
      if (cls.availableSeats === 0) row.classList.add('soldout');
      list.appendChild(row);
    }
    box.replaceChildren(list);
  });
}

function show(name, data) {
  const views = {
    login: loginView,
    register: registerView,
    home: () => homeView(data),
    profile: () => profileView(data),
    search: () => searchView(data),
  };
  app.replaceChildren(views[name]());
}
show('login');
