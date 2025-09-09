The project is basically a multithreaded HTTP/1.1 server that can handle multiple connections
concurrently,supports basic routes,file serving,file upload via a POST request,gzip compression and request logging on the server side.

Key features:
  Handles GET and POST requests.
  Serves static files from a given directory.
  Extracts and logs the User-Agent
  Persistent connections(kept alive until the user closes or sends a Connection: close header)
  Writes the access logging to the access.log file
  Thread per connection using the pthread library
  Supports /echo/<message> endpoint (with optional gzip encoding)
  Supports files/<filename> for both GET and POST requests
Build instructions:
   requirements:
      GCC or Clang
      zlib (-lz) for gzip compressions

   compiling:make all
   running the server: ./main

   The server listens on port 4221.
   By default, it serves files relative to the directory passed as an argument (if any).


supported endpoints:
   Root / : curl -v http://localhost:4221/
   
   Files /files/<filename>:
      GET : serves a file from the specified directory (for this you have to include the directory when running the server : ./main --directory)

      curl -v http://localhost:4221/files/test.txt

      POST:Uploads data into a file (same for the get request ,you have to provide the directory when running the server)

      curl -v -X POST http://localhost:4221/files/test.txt -d "Hello World"

   
Logging:
   All requests to this server are logged into the access.log file int the format:[YYYY-MM-DD HH:MM:SS] <client-ip> "<METHOD> <PATH> <VERSION>" <status> <response-size> "<User-Agent>"


Error Handling:
   404 Not Found for missing files or bad requests
   400 Bad request for malformed requests
   Graceful connection close when the user sends Connection: close


      
