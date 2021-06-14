Here is a brief summary for each assignment:

Assignment 1:
Take a look at folder 'application' here.
Notice that compareing the following two:
A. memcpy (&A, &B, sizeof(A));
B. memcpy (&A, &B, sizeof(B));
Option B is a better choice, because
using Option A there will be a silent bug
if size of A is smaller than size of B,
in which case only partial content of B
will be copied to A; using Option B under
the mentioned condition, the memory space
next to where A is will be overwritten,
and this bug is easier to observe, since
it may cause, say, segmentation fault.

Assignment 2:
Take a look at folder 'application' here.
Here I use the same trick as I did for
the solution for hw5. The main thread
sets up the periodic timers and uses
them to signal each child thread.
You may try the alternative approach
using multi-processing.

Assignment 3:
Take a look at folder 'service' here.
I called the prioridy queue EDFQ
(EDF queue). Also, see how I computed
the absolute deadline for each event,
between lines 169-201 in the .cpp file.

Assignment 4:
Take a look at folder 'service' here.
This part requires some more work. 
I called the FIFO queue FIFOQ.
I created a help class to keep our
worker thread. The class is named
'OurSingletonWorker' and is defined
in the .h file. We need the singleton
pattern here, because there will be
multiple TAO_EC_Default_ProxyPushConsumer
objects, one per supplier connection.
With five suppliers, if we do not
use singleton, the program will spawn
five worker threads with five independent
mutexes, etc.

Assignment 5:
Take a look at the PNG file.
The end-to-end latency curiously aligned
to 100 ms and 200 ms. Can you think of
a reason for that? It is because the
proxy thread will need to wait until
the next event arrival before it would
take another look at the FIFOQ.
In the final Homework 8 we will try to
resolve this issue :)

