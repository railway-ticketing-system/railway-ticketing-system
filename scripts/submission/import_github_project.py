"""Idempotently import repository-backed stories after granting gh the project scope.

Dates are course schedule fields. Creation and closure timestamps are never changed.
"""
import argparse,json,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];REPO='railway-ticketing-system/railway-ticketing-system'
def gh(*args,parse=True):
 p=subprocess.run(['gh',*map(str,args)],cwd=ROOT,text=True,capture_output=True)
 if p.returncode:raise RuntimeError(p.stderr.strip())
 return json.loads(p.stdout) if parse and p.stdout.strip() else p.stdout.strip()
def main():
 p=argparse.ArgumentParser();p.add_argument('--owner',default='railway-ticketing-system');a=p.parse_args()
 owner=a.owner; title='Railway Reservation System Course Delivery'
 projects=gh('project','list','--owner',owner,'--format','json','--limit','100')['projects']
 project=next((x for x in projects if x['title']==title),None)
 if not project:project=gh('project','create','--owner',owner,'--title',title,'--format','json')
 number=project['number'];pid=project['id'];gh('project','link',number,'--owner',owner,'--repo',REPO,parse=False)
 gh('project','edit',number,'--owner',owner,'--description','Retrospective course milestone reconstruction from original commits and PRs; Jira history remains the source for original sprint activity.','--readme',(ROOT/'docs/planning/BACKLOG.md').read_text(),parse=False)
 fields={x['name']:x for x in gh('project','field-list',number,'--owner',owner,'--format','json')['fields']}
 specs=[('Course sprint','SINGLE_SELECT',['Course Sprint 1','Course Sprint 2','Course Sprint 3']),('Story points retrospective','NUMBER',None),('Scheduled start','DATE',None),('Scheduled finish','DATE',None),('Requirement IDs','TEXT',None),('Original Jira ticket','TEXT',None),('Original Jira sprint','TEXT',None)]
 for name,typ,options in specs:
  if name in fields:continue
  args=['project','field-create',number,'--owner',owner,'--name',name,'--data-type',typ,'--format','json']
  if options:args+=['--single-select-options',','.join(options)]
  fields[name]=gh(*args)
 fields={x['name']:x for x in gh('project','field-list',number,'--owner',owner,'--format','json')['fields']}
 issues={x['title']:x for x in gh('issue','list','--repo',REPO,'--state','all','--limit','500','--json','number,title,url,state')}
 existing={x.get('content',{}).get('url'):x for x in gh('project','item-list',number,'--owner',owner,'--format','json','--limit','500')['items']}
 def value(item,name,kind,v):
  gh('project','item-edit','--id',item['id'],'--project-id',pid,'--field-id',fields[name]['id'],kind,v,parse=False)
 def select(item,name,option):
  opts=fields[name].get('options',[]); val=next(x['id'] for x in opts if x['name']==option);value(item,name,'--single-select-option-id',val)
 exported=[]
 for x in json.loads((ROOT/'docs/planning/backlog.json').read_text())['items']:
  heading=f"[{x['key']}] {x['title']}"; issue=issues.get(heading) or next((v for k,v in issues.items() if k.startswith('['+x['key']+'] ')),None)
  if not issue:
   body=f"Reconstructed on 2026-11-12 from existing repository evidence. This GitHub issue was created at import time, not on the historical development date.\n\nOriginal Jira: {x['jira_url']}\n\nRequirements: {x['requirements']}\n\nImplementation commit: https://github.com/{REPO}/commit/{x['commit']}\nOriginal author/date: {x['commit_author']} / {x['actual_commit_date']}\n\nCourse milestone: {x['course_sprint']} ({x['planned_start']} to {x['planned_end']}). This is a course milestone mapping, not verified original Jira sprint membership.\nStory points: {x['story_points']} ({x['estimate_basis']}).\n\nRelated PRs:\n"+'\n'.join(f'- https://github.com/{REPO}/pull/{n}' for n in x['pull_requests'])+'\n\nStatus reflects existing implementation delivery. Human acceptance is tracked separately. Original Jira descriptions/estimates will be reconciled once Jira is connected.\n'
   with tempfile.NamedTemporaryFile('w',suffix='.md') as f:
    f.write(body);f.flush();url=gh('issue','create','--repo',REPO,'--title',heading,'--assignee',x['assignee'],'--body-file',f.name,parse=False)
   issue={'url':url,'state':'OPEN'}
  if issue['state']=='OPEN':gh('issue','close',issue['url'],'--reason','completed',parse=False)
  item=existing.get(issue['url']) or gh('project','item-add',number,'--owner',owner,'--url',issue['url'],'--format','json')
  select(item,'Course sprint',x['course_sprint']);select(item,'Status','Done')
  value(item,'Story points retrospective','--number',str(x['story_points']))
  for name,key in [('Scheduled start','planned_start'),('Scheduled finish','planned_end')]:value(item,name,'--date',x[key])
  value(item,'Original Jira sprint','--text',', '.join(z['name'] for z in x.get('original_jira_sprints',[])))
  value(item,'Requirement IDs','--text',x['requirements']);value(item,'Original Jira ticket','--text',x['jira_url'])
  exported.append({'key':x['key'],'issue':issue['url'],'project_item_id':item['id']});print(x['key'],issue['url'],flush=True)
 for n in range(2,13):
  url=f'https://github.com/{REPO}/pull/{n}'
  if url not in existing:gh('project','item-add',number,'--owner',owner,'--url',url,'--format','json')
 result={'project':project,'imported_items':exported,'creation_time_is_actual_import_time':True}
 (ROOT/'docs/evidence/github-project-import.json').write_text(json.dumps(result,indent=2)+'\n')
 print('Verified project:',project.get('url',f'https://github.com/orgs/{owner}/projects/{number}'))
if __name__=='__main__':
 try:main()
 except (RuntimeError,StopIteration) as e:print(f'Import stopped: {e}. Grant GitHub project scope with gh auth refresh -s project, then rerun. Existing completed steps can be reused.',file=sys.stderr);sys.exit(1)
