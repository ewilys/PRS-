# PRS-
preparation for prs project

client and serveur with .h :
- setting up a file buffer in each program to open the file in once and writes data in the buffer.
- serveur waits for client ack before sending others fragments.
- client writes the data in the file only when he receives all the segment.
- setting up a specific header with : number of the fragment, fragmentation flag (0=last) and size of the payload.
- first segment sent by serveur is the size of the file . (payload= size of the file) segment numero 0. than serveur starts to send the fragments from 1 to (sizeFile/MDS)+1 . Indeed, all the file is sent when number of segment = sizeFile/MDS+1.


Careful maybe add an idFile in the header if different files are sent meanwhile ??
Maybe try to write all data in the all_file_buffer in once rather than in a loop.


No tcp protocols implanted.
