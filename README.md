# Mutual exclusion in distributed system.

Every process (knight) in distrubuted system requests one of available resources (windmills) 
but only 4 of them are allowed to use the resource simultaneously. 
Every process goes in loop:
* it request access to critical section
* stays in critical section for random time
* releases critical section and waits for a random time  
Only 4 processes are allowed to be in critical section at the same time.

Code is implementation of modified Lamport algorithm written in [OpenMPI](https://www.open-mpi.org/) . 

When running the application you need to pass number of available resources as parameter.


