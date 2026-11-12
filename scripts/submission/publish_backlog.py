"""Publish repository-backed backlog/milestones even without GitHub Projects permission."""
from pathlib import Path
import json,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2];REPO='railway-ticketing-system/railway-ticketing-system'
def gh(*args,parse=True):
 r=subprocess.run(['gh',*map(str,args)],capture_output=True,text=True,cwd=ROOT,check=True)
 return json.loads(r.stdout) if parse and r.stdout.strip() else r.stdout.strip()
def main():
 items=json.loads((ROOT/'docs/planning/backlog.json').read_text())['items']
 for label,color,desc in [('retrospective-record','c5def5','Reconstructed now from existing commits and PRs; original timestamps retained'),('points:3-retrospective','d4c5f9','3-point estimate made during submission reconstruction; original Jira estimate unverified'),('points:5-retrospective','d4c5f9','5-point estimate made during submission reconstruction; original Jira estimate unverified')]:
  subprocess.run(['gh','label','create',label,'--repo',REPO,'--color',color,'--description',desc,'--force'],cwd=ROOT,check=True,capture_output=True)
 milestones={m['title']:m for m in gh('api',f'repos/{REPO}/milestones?state=all&per_page=100')}
 for name,start,end in [('Course Sprint 1','2026-10-19','2026-10-23'),('Course Sprint 2','2026-10-26','2026-10-30'),('Course Sprint 3','2026-11-02','2026-11-06')]:
  if name not in milestones:milestones[name]=gh('api','-X','POST',f'repos/{REPO}/milestones','-f',f'title={name}','-f',f'due_on={end}T23:59:59Z','-f',f'description=Course scheduled window {start} through {end}. Reconstructed on 2026-11-12 from original commits/PRs; original Jira sprint membership/estimates not yet imported. GitHub creation/closure dates remain actual import dates.')
 issues={x['title']:x for x in gh('issue','list','--repo',REPO,'--state','all','--limit','500','--json','number,title,url,state')}
 published=[]
 for x in items:
  title=f"[{x['key']}] {x['title']}";i=issues.get(title) or next((v for k,v in issues.items() if k.startswith('['+x['key']+'] ')),None)
  if not i:
   body=f"## Existing implementation evidence\n\nOriginal Jira ticket: {x['jira_url']}\n\nRequirements: {x['requirements']}\n\nCommit: https://github.com/{REPO}/commit/{x['commit']}\nAuthor and actual date: {x['commit_author']} — {x['actual_commit_date']}\n\nRelated PRs:\n"+'\n'.join(f'- https://github.com/{REPO}/pull/{n}' for n in x['pull_requests'])+f"\n\n## Course milestone reconstruction\n\nCourse schedule: {x['course_sprint']}, {x['planned_start']} through {x['planned_end']}.\n\nEstimate: **{x['story_points']} points**, newly estimated during reconstruction; not verified original Jira points. Original Jira description and sprint metadata remain to be reconciled after connection.\n\nCreated on 2026-11-12 as a retrospective record of implementation already present in the repository. GitHub timestamps are actual publication/closure times. Closing this issue records existing implementation, not a newly manufactured historical PR, independent review or human acceptance.\n"
   with tempfile.NamedTemporaryFile('w',suffix='.md') as f:
    f.write(body);f.flush();url=gh('issue','create','--repo',REPO,'--title',title,'--body-file',f.name,'--assignee',x['assignee'],'--milestone',x['course_sprint'],'--label','retrospective-record','--label',f"points:{x['story_points']}-retrospective",parse=False)
   i={'url':url,'state':'OPEN'}
  if i['state']=='OPEN':gh('issue','close',i['url'],'--reason','completed',parse=False)
  published.append({'key':x['key'],'url':i['url'],'course_sprint':x['course_sprint']})
  (ROOT/'docs/evidence/github-backlog-import.json').write_text(json.dumps({'published_on':'2026-11-12','issues':published,'milestones':list(milestones.values()),'basis':'Repository evidence; original Jira metadata not yet verified'},indent=2)+'\n')
  print(x['key'],i['url'],flush=True)
 print(f'Published {len(published)} backlog records with assignees, estimates, course milestones and evidence.')
if __name__=='__main__':main()
