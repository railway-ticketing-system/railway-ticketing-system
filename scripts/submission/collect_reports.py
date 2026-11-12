"""Rebuild tests and regenerate submission evidence; every failed step fails the command."""
import argparse, os, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def run(args,**kw):
 print('+',' '.join(map(str,args)),flush=True);subprocess.run(list(map(str,args)),cwd=ROOT,check=True,**kw)
def main():
 p=argparse.ArgumentParser();p.add_argument('--build-dir',default='build-submission');p.add_argument('--jobs',default='4');a=p.parse_args()
 out=ROOT/'docs/reports';out.mkdir(parents=True,exist_ok=True);b=ROOT/a.build_dir
 run(['cmake','-S','.', '-B',b,'-DCMAKE_BUILD_TYPE=Debug','-DRTRS_COVERAGE=ON'])
 run(['cmake','--build',b,'-j',a.jobs])
 # Reset only generated counter files in the chosen build tree to avoid stale accumulation.
 for f in b.rglob('*.gcda'):f.unlink()
 run([b/'rtrs_tests','--gtest_output=xml:'+str(out/'unit-tests.xml')])
 run(['gcovr','--root','.',b,'--filter','src/','--exclude','src/main.cpp','--gcov-ignore-parse-errors','negative_hits.warn_once_per_file','--json',out/'coverage.json','--json-summary',out/'coverage-summary.json','--html-details',out/'coverage.html','--txt',out/'line-coverage.txt','--print-summary'],stdout=(out/'coverage-collection.log').open('w'),stderr=subprocess.STDOUT)
 run([sys.executable,ROOT/'scripts/submission/build_reports.py'])
 if (ROOT/'scripts/submission/system_validation.py').exists():run([sys.executable,ROOT/'scripts/submission/system_validation.py','--binary',b/'rtrs_server'])
if __name__=='__main__':main()
