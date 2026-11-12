"""Render the project's UML diagrams without a network service."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import math, textwrap
OUT=Path('docs/diagrams'); OUT.mkdir(parents=True,exist_ok=True)
FONT='/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf'
if not Path(FONT).exists(): FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
def canvas(w,h):
 im=Image.new('RGB',(w,h),'white'); return im,ImageDraw.Draw(im)
def text(d,xy,s,size=22):
 font=ImageFont.truetype(FONT,size)
 bounds=d.multiline_textbbox((0,0),s,font=font,anchor='mm',align='center',spacing=6)
 half=(bounds[2]-bounds[0])/2
 xy=(max(half+24,min(d._image.width-half-24,xy[0])),xy[1])
 d.multiline_text(xy,s,font=font,fill='#172b4d',anchor='mm',align='center',spacing=6)
def box(d,r,s):
 d.rounded_rectangle(r,radius=8,outline='#294e73',width=3,fill='#eef4fa')
 size=28
 while max(ImageFont.truetype(FONT,size).getlength(line) for line in s.split('\n'))>r[2]-r[0]-20:size-=1
 text(d,((r[0]+r[2])/2,(r[1]+r[3])/2),s,size)
def arrow(d,a,b,label='',offset=0):
 d.line([a,b],fill='#294e73',width=3);ang=math.atan2(b[1]-a[1],b[0]-a[0]);L=13
 d.polygon([b,(b[0]-L*math.cos(ang-.4),b[1]-L*math.sin(ang-.4)),(b[0]-L*math.cos(ang+.4),b[1]-L*math.sin(ang+.4))],fill='#294e73')
 if label:text(d,((a[0]+b[0])/2,(a[1]+b[1])/2+offset),label,24)
def actor(d,x,y,name):
 d.ellipse((x-15,y-65,x+15,y-35),outline='#294e73',width=3);d.line([(x,y-35),(x,y+10),(x-25,y+45)],fill='#294e73',width=3);d.line([(x,y+10),(x+25,y+45)],fill='#294e73',width=3);d.line([(x-30,y-10),(x+30,y-10)],fill='#294e73',width=3);text(d,(x,y+75),name)
im,d=canvas(1500,1080);text(d,(750,40),'Railway reservation use cases',30);d.rectangle((300,95,1200,1010),outline='#294e73',width=3);text(d,(750,125),'Railway Ticket Reservation System',22);actor(d,150,450,'Passenger');actor(d,1350,450,'Administrator')
items=[('Register and manage profile',510,220),('Log in and log out',990,220),('Search and view schedules',510,390),('View fares and availability',990,390),('Book and view ticket',510,560),('View booking history',990,560),('Cancel own ticket and view refund',510,730),('Manage train records',990,730),('Read booking audit trail',990,900)]
for label,x,y in items:
 d.ellipse((x-195,y-50,x+195,y+50),outline='#294e73',width=3,fill='#eef4fa');text(d,(x,y),label,24)
 if 'Manage train' in label or 'audit trail' in label:d.line([(1318,450),(x+195,y)],fill='#526f8d',width=2)
 else:d.line([(180,450),(x-195,y)],fill='#526f8d',width=2)
im.save(OUT/'use_cases.png')
im,d=canvas(1500,850);text(d,(750,40),'Layered architecture component diagram',30)
box(d,(440,95,1060,210),'<<component>> Browser UI\nHTML CSS JavaScript');box(d,(350,290,1150,405),'<<component>> C++17 REST routes\ncpp-httplib and nlohmann JSON');box(d,(80,500,500,625),'<<component>> User and Auth services\nOpenSSL hashes and sessions');box(d,(540,500,960,625),'<<component>> Train Booking Admin\nand Audit services');box(d,(1000,500,1420,625),'<<component>> Validation TLS and Log\nCross cutting controls');box(d,(350,710,1150,815),'<<component>> Database RAII wrapper\nSQLite prepared statements and transactions')
arrow(d,(750,210),(750,290),'HTTPS JSON',-8);arrow(d,(500,405),(290,500));arrow(d,(750,405),(750,500));arrow(d,(1040,405),(1210,500));arrow(d,(290,625),(550,710));arrow(d,(750,625),(750,710));im.save(OUT/'components.png')
def sequence(name,title,participants,msgs):
 width=1500;height=180+len(msgs)*84;im,d=canvas(width,height);text(d,(750,35),title,30);xs=[120+i*(1260/(len(participants)-1)) for i in range(len(participants))]
 for x,p in zip(xs,participants):
  box(d,(x-100,80,x+100,145),p)
  for y in range(150,height-20,18):d.line([(x,y),(x,y+10)],fill='#8ca3bc',width=2)
 for idx,(a,b,label) in enumerate(msgs):
  y=205+idx*84;arrow(d,(xs[a],y),(xs[b],y));text(d,((xs[a]+xs[b])/2,y-34),textwrap.fill(label,36),27)
 im.save(OUT/name)
sequence('booking_sequence.png','Ticket booking sequence',['Passenger UI','REST routes','AuthService','BookingService','SQLite'],[(0,1,'POST /api/bookings with bearer token'),(1,2,'Authenticate current session'),(2,1,'Return user identity'),(1,3,'Confirm validated passenger request'),(3,4,'BEGIN IMMEDIATE and recheck inventory'),(3,4,'Insert booking passengers and unique seats'),(3,4,'Insert audit event then COMMIT'),(3,1,'Return booking with unique PNR'),(1,0,'201 ticket and assigned seats')])
sequence('cancellation_sequence.png','Ticket cancellation sequence',['Passenger UI','REST routes','AuthService','BookingService','SQLite'],[(0,1,'GET cancellation eligibility and refund'),(1,2,'Authenticate and resolve booking owner'),(1,3,'Quote using configured cutoff and percent'),(3,1,'Return eligibility and refund amount'),(0,1,'PUT /api/bookings/{id}/cancel'),(1,3,'Recheck owner and eligibility'),(3,4,'BEGIN IMMEDIATE update status release seats'),(3,4,'Insert audit event then COMMIT'),(3,1,'Return cancelled ticket and stored refund'),(1,0,'200 cancellation confirmation')])
print('Rendered four UML diagrams')
