# DevRandom

### Motivation
I wrote this to play with and learn about Microsoft's new (at the time) thread pool API. 
Providing a windows version of the unix /dev/random seemed like a good way to exercise the API.

-----

#### Goals:
  1. Provide high quality random data to as many clients as possible as fast as possible whilt minimizing impact on the system.
  2. Prevent malicious clients from creating DoS attacks.
  3. Provide a file in the file system that can be read for random data like /dev/random on unix.

----
#### Approach:
  1. Use Thread Pool and Asynchronous file API's to maximize output while minimizing impact on the processor. 
     Used Microsoft's Crypto API to generate high-quality random data.
  2. To ensure the system remains responsize regardless of how many clients connect, 
     the thread pool is initialized with the number of cores - 1 threads. 
     This ensures that regardless of how many clients connect, at least one core remains free.
  3. The DevRandom application creates a named pipe \\.\pipe\random. This pipe can be connected to directly. 
     Alternativly, the mklink utility can be used to create a symbolic link from the file system
     to \\localhost\pipe to create a file descriptor from which random data can be read. Note: 
     to make this work, you need to use fsutil to enable symbolic links from a local file system to a 
     remote file system. Note: mklink will allow you to create a symbolic link from the file system to
     \\.\pipe\random. However, I found that I was unable to get data out of that link. If I try to type 
     or cat a link that points to \\.\pipe\random, I get the error: 
     "The data present in the reparse point buffer is invalid."

---

#### Projects:
  1. ThreadPool: This project creates a dll that provides a C++ API to access the Thread Pool features.
  2. DevRandom: This project creates al ATL based service executable that links with ThreadPool to create 
     the random data service.
  3. QuickSort: This was another experimental project that used ThreadPool to implement QuickSort using
     the ThreadPool API and compare performance to a single threaded version. Turns out the Thread Pool 
     version of QuickSort is much slower. I did very little work on this and will probably remove it at
     some point.

---

#### Results:
  It seemed to work out quite well. You can get as much random data as you like by simply opening and reading
  the pipe. Even cmd.exe TYPE command works. To test performance, I created up to 200 instances of the command
  type \\.\pipe > nul. The results were that on a 4 core system, I was able to get somewhere aroung 200MB per 
  second of random data. As the number of clients increases, they all slow down equally, so no client is favored.
  Further, no matter how many clients I created, the system remains responsive. 

#### TODO:
 I initially wrote this using Visual C++ 2010 At this point, I'd like to go back and update it to use C++ 11 standards, 
 although there is little motivation for this as the service seems to work just fine as is. At some point, I may add 
 a batch file that creates the symbolic links from \dev\random and \\dev\urandom to \\pipe\localhost\random. 