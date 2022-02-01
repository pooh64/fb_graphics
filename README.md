This repo contains a small software rasterizer that does not depend on any external graphics/computing APIs and runs only on the CPU

### examples/test0
220fps 1080p (Intel i5-8250U 4cores 8threads)
![test0png](test0.png)  

#### VTune shows a very good use of processor caches even on my old Intel 5th gen L1=64K CPU, textures take up 28MB   
![test0vtune](test0_vtune.png)  
### References
#### Sources of optimizations which have been adapted to CPU rasterizer in this project
https://research.nvidia.com/publication/high-performance-software-rasterization-gpus  
https://www.cs.cmu.edu/afs/cs/academic/class/15869-f11/www/readings/abrash09_lrbrast.pdf  

### examples/test1
Perform raycasting reusing flexible source code
![test1png](test1.png)
