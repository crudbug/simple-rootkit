trusting-trust-simple
=====================

A simple example "trusting trust" type attack via kernel module, with highly detailed comments.

Here we'll compile a kernel module which intercepts every "read" system call, searches for a string that looks like common C source code, and attaches something new to the source. This is meant to demonstrate how a compromised system can build a malicious binary from perfectly safe source code.

For more information about "Trusting Trust" attacks see: https://www.schneier.com/blog/archives/2006/01/countering_trus.html

###Instructions

Install your kernel headers

  sudo apt-get install linux-headers-$(uname -r)

Run make

  cd trusting-trust-simple && make
  
Load the module

  sudo insmod trusting-trust-simple.ko
  
Compile any C program and it should now start with a message.

  gcc hello.c -o hello
  ./hello
  


