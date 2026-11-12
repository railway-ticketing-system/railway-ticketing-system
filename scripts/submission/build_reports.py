"""Build readable unit, line and branch reports from genuine XML and gcovr output."""
from pathlib import Path
import json, platform, subprocess, xml.etree.ElementTree as ET
from datetime import datetime,timezone
ROOT=Path(__file__).resolve().parents[2]; OUT=ROOT/'docs/reports'
def cmd(*args):return subprocess.check_output(args,cwd=ROOT,text=True).strip()
def main():
 root=ET.parse(OUT/'unit-tests.xml').getroot(); cov=json.loads((OUT/'coverage-summary.json').read_text())
 meta={'generated_at':datetime.now(timezone.utc).isoformat(),'source_commit':cmd('git','rev-parse','HEAD'),'platform':platform.platform(),'compiler':cmd('g++','--version').splitlines()[0],'gcovr':cmd('gcovr','--version').splitlines()[0]}
 (OUT/'execution-metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
 common=f"Generated {meta['generated_at']}. Application source: `{meta['source_commit']}`.\n\nEnvironment: {meta['platform']}; {meta['compiler']}; {meta['gcovr']}.\n\n"
 text='# Unit Test Report\n\n'+common
 text+=f"GoogleTest executed **{root.get('tests')} tests** in **{len(root.findall('testsuite'))} suites**: **{root.get('failures')} failures**, **{root.get('errors')} errors**, {root.get('disabled')} disabled; reported duration {root.get('time')} seconds. This binary includes unit, service, API integration, concurrency and search performance tests; the total is not a count of distinct Test Plan cases.\n\n"
 text+='Built freshly with CMake Debug, GCC and `RTRS_COVERAGE=ON`. Evidence: [GoogleTest XML](unit-tests.xml), [CTest JUnit](ctest-results.xml), [system validation](System_Validation_Report.md). Test hashes use reduced PBKDF2 iterations for speed; the system runner separately exercises the executable configuration.\n\n| Suite | Tests | Failures | Time seconds |\n|---|---:|---:|---:|\n'
 for s in root.findall('testsuite'):text+=f"| {s.get('name')} | {s.get('tests')} | {s.get('failures')} | {s.get('time')} |\n"
 text+='\n## Reproduction\n\nRun `python3 scripts/submission/collect_reports.py --build-dir build-submission --jobs 4` with CMake, GCC, OpenSSL, gcovr 8.8 and Python available. An initial dependency download requires network access.\n\n## Interpretation\n\nThese results validate the selected scenarios on one local Linux machine. They do not establish production readiness, hosted deployment, penetration-test certification, or human UAT acceptance. See the System Validation Report and submission checklist for remaining human actions.\n'
 (OUT/'Unit_Test_Report.md').write_text(text)
 for kind,title in [('line','Code Coverage Report'),('branch','Branch Coverage Report')]:
  covered,total=cov[kind+'_covered'],cov[kind+'_total'];percent=cov[kind+'_percent']
  s='# '+title+'\n\n'+common+f'**{percent}%**: **{covered} / {total}** measured {kind}s covered.\n\n'
  s+='## Scope and method\n\nGCC/gcov instrumentation with gcovr, restricted to project `src/` files and headers; `src/main.cpp`, tests and downloaded dependencies are excluded. The executable startup, CLI and real TLS path are additionally exercised by the system validation runner but are not included in this coverage percentage. No exclusions for exception branches or unreachable branches are applied. Compiler-generated C++ exception/cleanup edges remain in the branch denominator. No minimum percentage was specified in the five course PDFs.\n\n'
  s+='Negative-hit parse handling matches the existing CI workaround: `--gcov-ignore-parse-errors negative_hits.warn_once_per_file`. The collection log is included as `coverage-collection.log`; inspect warnings when regenerating.\n\n'
  s+='[Interactive source coverage](coverage.html) · [Raw coverage JSON](coverage.json) · [Summary JSON](coverage-summary.json)\n\n| File | Covered | Total | Percent |\n|---|---:|---:|---:|\n'
  for f in cov['files']:s+=f"| `{f['filename']}` | {f[kind+'_covered']} | {f[kind+'_total']} | {f[kind+'_percent'] if f[kind+'_percent'] is not None else 'N/A'} |\n"
  s+='\n## Follow up\n\nPrioritize untested error and rollback paths visible in the HTML report. A passing test suite is not equivalent to complete branch coverage.\n'
  (OUT/('Code_Coverage_Report.md' if kind=='line' else 'Branch_Coverage_Report.md')).write_text(s)
 print(f"Reports generated: {root.get('tests')} tests; line {cov['line_percent']}%; branch {cov['branch_percent']}%")
if __name__=='__main__':main()
