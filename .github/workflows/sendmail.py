import sys
import smtplib

if len(sys.argv) < 4:
  print ('sendemail User Pass To < Message')
  sys.exit(1)
#
# sys.argv[0] Command
# sys.argv[1] Email UserName
# sys.argv[2] Email Password
# sys.argv[3] To address
# stdin       Subject + body
#

try:
  User = sys.argv[1]
  Pass = sys.argv[2]
  To   = sys.argv[3]
  Body = sys.stdin.read()
  Message = "From: %s\nTo: %s\n%s" % (User, To, Body)

  server = smtplib.SMTP('smtp.gmail.com', 587)
  server.starttls()
  server.ehlo()
  server.login(User, Pass)
  server.sendmail(User, To, Message)
except:
  print ('ERROR: Can not send email')
  sys.exit(1)
