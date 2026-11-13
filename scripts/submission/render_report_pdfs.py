"""Render the four required test reports as printable PDFs from the committed Markdown."""
from pathlib import Path
import re,html
from reportlab.platypus import SimpleDocTemplate,Paragraph,Spacer,LongTable,TableStyle,KeepTogether
from reportlab.lib.styles import getSampleStyleSheet,ParagraphStyle
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
ROOT=Path(__file__).resolve().parents[2]
FONT=Path('/usr/share/fonts/dejavu-sans-fonts')
if not FONT.exists():FONT=Path('/usr/share/fonts/truetype/dejavu')
pdfmetrics.registerFont(TTFont('DejaVu',str(FONT/'DejaVuSans.ttf')))
pdfmetrics.registerFont(TTFont('DejaVu-Bold',str(FONT/'DejaVuSans-Bold.ttf')))
pdfmetrics.registerFontFamily('DejaVu',normal='DejaVu',bold='DejaVu-Bold',italic='DejaVu',boldItalic='DejaVu-Bold')
styles=getSampleStyleSheet()
for name in ['Normal','Title','Heading1','Heading2','Heading3']:
 styles[name].fontName='DejaVu';styles[name].textColor=colors.HexColor('#182b43')
styles['Normal'].fontSize=9;styles['Normal'].leading=13;styles['Normal'].spaceAfter=6
styles['Title'].fontSize=21;styles['Title'].leading=26
styles['Heading2'].fontSize=13;styles['Heading2'].leading=17;styles['Heading2'].spaceBefore=12
styles.add(ParagraphStyle(name='Cell',fontName='DejaVu',fontSize=7.6,leading=9.2,splitLongWords=True))
styles.add(ParagraphStyle(name='CodeSmall',fontName='DejaVu',fontSize=7.5,leading=10,splitLongWords=True))
def inline(s):
 s=html.escape(s)
 s=re.sub(r'\[([^\]]+)\]\(([^)]+)\)',r'\1',s)
 s=re.sub(r'\*\*(.+?)\*\*',r'<b>\1</b>',s);s=s.replace('`','')
 return s

def render(path):
 lines=path.read_text().splitlines();story=[];i=0;code=False
 while i<len(lines):
  s=lines[i].strip()
  if not s:i+=1;continue
  if s.startswith('```'):code=not code;i+=1;continue
  if code:story.append(Paragraph(inline(s),styles['CodeSmall']));i+=1;continue
  if s.startswith('|'):
   rows=[]
   while i<len(lines) and lines[i].strip().startswith('|'):
    row=[x.strip() for x in lines[i].strip().strip('|').split('|')];i+=1
    if all(re.fullmatch(r':?-+:?',x) for x in row):continue
    rows.append(row)
   n=max(map(len,rows));width=A4[0]-84
   if n==3:widths=[width*.16,width*.69,width*.15] if rows[0][0]=='Check ID' else [width*.65,width*.175,width*.175]
   elif n==4:widths=[width*.61,width*.13,width*.13,width*.13]
   else:widths=[width/n]*n
   table=LongTable([[Paragraph(inline(x),styles['Cell']) for x in r] for r in rows],colWidths=widths,repeatRows=1,hAlign='LEFT')
   table.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),colors.HexColor('#e8eef5')),('VALIGN',(0,0),(-1,-1),'TOP'),('LINEBELOW',(0,0),(-1,0),.5,colors.HexColor('#8396ad')),('ROWBACKGROUNDS',(0,1),(-1,-1),[colors.white,colors.HexColor('#f5f7fa')]),('BOTTOMPADDING',(0,0),(-1,-1),2),('TOPPADDING',(0,0),(-1,-1),2)]));story+=[table,Spacer(1,10)];continue
  if s.startswith('# '):story.append(Paragraph(inline(s[2:]),styles['Title']))
  elif s.startswith('## '):story.append(Paragraph(inline(s[3:]),styles['Heading2']))
  elif s.startswith('### '):story.append(Paragraph(inline(s[4:]),styles['Heading3']))
  else:story.append(Paragraph(inline(s[2:] if s.startswith('- ') else s),styles['Normal']))
  i+=1
 def page(c,d):
  c.setFont('DejaVu',8);c.setFillColor(colors.HexColor('#52637b'));c.drawString(42,24,'Railway Ticket Reservation System | Verification evidence');c.drawRightString(A4[0]-42,24,str(d.page))
 out=path.with_suffix('.pdf');SimpleDocTemplate(str(out),pagesize=A4,rightMargin=42,leftMargin=42,topMargin=42,bottomMargin=42,title=path.stem.replace('_',' '),author='Deeptanshu Kumar').build(story,onFirstPage=page,onLaterPages=page)
 print(out)
if __name__=='__main__':
 for name in ['Unit_Test_Report','System_Validation_Report','Code_Coverage_Report','Branch_Coverage_Report']:render(ROOT/'docs/reports'/f'{name}.md')
