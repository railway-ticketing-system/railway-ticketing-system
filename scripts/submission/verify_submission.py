"""Validate submission artifact consistency; does not certify human acceptance."""
import json,re,sys,zipfile,xml.etree.ElementTree as ET
from pathlib import Path
from datetime import datetime,timezone
ROOT=Path(__file__).resolve().parents[2]
def main():
 checks=[]
 def check(name,passed,detail=''):checks.append(dict(check=name,status='PASS' if passed else 'FAIL',detail=detail))
 required=['docs/SUBMISSION_INDEX.md','docs/Railway_Ticket_Reservation_System_SRS.docx','docs/Railway_Ticket_Reservation_System_SRS.pdf','docs/Railway_Ticket_Reservation_System_SAD_Final.docx','docs/Railway_Ticket_Reservation_System_SAD_Final.pdf','docs/Railway_Ticket_Reservation_System_Test_Plan_Final.docx','docs/Railway_Ticket_Reservation_System_Test_Plan_Final.pdf','docs/DEVIATIONS.md','docs/planning/BACKLOG.md','docs/planning/FEATURE_FREEZE.md','docs/evidence/github-backlog-import.json','docs/demo/LIVE_DEMO_RUNBOOK.md','docs/demo/VIVA_PREP.md','docs/demo/VIDEO_HANDOFF.md','docs/reports/Browser_Validation_Report.md']
 for name in ['Unit_Test_Report','System_Validation_Report','Code_Coverage_Report','Branch_Coverage_Report']:
  required.extend([f'docs/reports/{name}.md',f'docs/reports/{name}.pdf'])
 for name in required:
  p=ROOT/name;check('Artifact '+name,p.is_file() and p.stat().st_size>0)
 missing=[]
 for p in [ROOT/'README.md',*list((ROOT/'docs').rglob('*.md'))]:
  for target in re.findall(r'\[[^\]]*\]\(([^)]+)\)',p.read_text()):
   if '://' in target or target.startswith(('#','mailto:')):continue
   target=target.split('#')[0].split(' "')[0].strip('<>')
   if target and not (p.parent/target).exists():missing.append(f'{p.relative_to(ROOT)} -> {target}')
 check('All local Markdown links resolve',not missing,missing)
 req=json.loads((ROOT/'docs/source/requirements.json').read_text());cases=json.loads((ROOT/'docs/source/test_cases.json').read_text());check('31 FR NFR and security requirements',len(req)==31,len(req));check('At least ten detailed test cases',len(cases)>=10,len(cases))
 caseids={c['id'] for c in cases};check('Every requirement maps to existing cases',all(c in caseids for r in req for c in r['cases']))
 unit=ET.parse(ROOT/'docs/reports/unit-tests.xml').getroot();names={f"{s.get('name')}.{t.get('name')}"for s in unit.findall('testsuite')for t in s.findall('testcase')}
 unknown=[(c['id'],t)for c in cases for t in c['tests'] if t not in names];check('Named automated tests exist in executed XML',not unknown,unknown);check('Unit run has no errors or failures',int(unit.get('errors','0'))+int(unit.get('failures','0'))==0,unit.get('tests'))
 system=json.loads((ROOT/'docs/reports/system_validation.json').read_text());check('System run complete and passing',system['status']=='PASS' and not system.get('incomplete'),system['summary'])
 cov=json.loads((ROOT/'docs/reports/coverage-summary.json').read_text());check('Coverage counts within totals',all(0<=cov[k+'_covered']<=cov[k+'_total']for k in ['line','branch','function']),{k:cov[k+'_percent']for k in ['line','branch','function']})
 for file,min_images in [('Railway_Ticket_Reservation_System_SRS.docx',1),('Railway_Ticket_Reservation_System_SAD_Final.docx',3)]:
  p=ROOT/'docs'/file
  if p.exists():
   with zipfile.ZipFile(p) as z:
    n=z.read('word/document.xml').count(b'<wp:inline')+z.read('word/document.xml').count(b'<wp:anchor')
   check(file+' embedded diagrams',n>=min_images,n)
 board=ROOT/'docs/evidence/github-project-import.json'
 human=['Friend uploads 60-second and 120-second videos','Amar performs final human acceptance and reviews/merges submission','Product owner/mentor record actual acceptance','Connect Jira or supply current export to reconcile the recovered13Oct snapshot with later dates/statuses and original story points']
 if not board.exists():human.append('Grant GitHub Projects scope and run visual board importer')
 result={'checked_at':datetime.now(timezone.utc).isoformat(),'status':'PASS' if all(c['status']=='PASS'for c in checks)else 'FAIL','checks':checks,'external_actions_pending':human,'meaning':'Artifact consistency only; pending external actions prevent claiming the whole course submission accepted'}
 (ROOT/'docs/evidence/submission-validation.json').write_text(json.dumps(result,indent=2)+'\n')
 for c in checks:
  if c['status']=='FAIL':print('FAIL:',c['check'],c['detail'])
 print(f"Artifact validation {result['status']}: {sum(c['status']=='PASS'for c in checks)}/{len(checks)} checks. {len(human)} external actions remain.")
 return 0 if result['status']=='PASS'else 1
if __name__=='__main__':sys.exit(main())
