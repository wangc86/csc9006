# CSC9006 Real-Time Systems, Lab 1

This lab assignment is an integrated part of the [real-time systems](https://wangc86.github.io/csc9006/) course taught at [National Taiwan Normal University](https://www.ntnu.edu.tw/). You have two weeks to complete this lab. The hard deadline is Friday 10 PM, 3/27/2020.

In class, we introduced two classic real-time scheduling algorithms: RM and EDF. We gave a simple, two-task example to illustrate how the algorithms work, and we had a glimpse on the use of utilization bound for a schedulability test. Now, let's see how RM and EDF work in practice.

In this lab, we will go through an _empirical evaluation_ of RM and EDF, using the same two-task example. Throughout the process, we will

* use the C++11 standard library `chrono` to measure the response time of a program
* create some synthetic workload for our evaluation purpose
* leverage linux's support for real-time computing (`man sched`)
* make experimental plans for performance evaluation
* visualize experimental results (using Matplotlib)

In the following, the text in **bold** highlights the materials that you should submit to Moodle or your GitHub repository.

Let's begin :)

## Measuring the response time of a program

In real-time systems, we often care about the response time of a program. Among various time utilities, I found C++'s `chrono` library in particular suitable for our need. There are at least two great C++ reference sites where you may learn about the library: [cppreference.com](https://en.cppreference.com/w/cpp/chrono) and [cplusplus.com](http://www.cplusplus.com/reference/chrono/?kw=chrono). Spend some time to get yourself familiar with the sites' interfaces.

