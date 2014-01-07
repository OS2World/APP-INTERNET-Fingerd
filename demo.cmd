/* REXX Script */
cr='0d'x
Say 'Demo of Aaron''s Finger Daemon:' || cr
Say cr
Say 'Before you begin you must have IBM TCP/IP installed.' || cr
Say 'This program has only been tested with version 2.0' || cr
Say 'Press Enter to continue' || cr
Pull Dummy
Say 'Starting the finger daemon...' || cr
'start fingerd.exe -f finger.dat -d -b -l finger.log'
Say 'Delaying 5 seconds to wait for daemon to start... '
'@sleep 5000'
Say 'The finger daemon has been started by now, press Enter to finger your host.' || cr
Pull dummy
'finger %HOSTNAME%'
Say 'Press any key when ready.' || cr || cr
Pull dummy
Say 'This finger request has been logged.  Press Enter to see the log file.'
Pull Dummy
'type finger.log'
Say cr
Say cr
Say 'See fingerd.txt for documentation, or just type fingerd without any' || cr
Say 'parameters to see a list of options.' || cr
Say 'To kill the finger daemon type ^C in the OS/2 command session running' || cr
Say 'FINGERD.EXE' || cr || cr
Say 'See the file fingerd.txt for information on installing this program.' || cr

