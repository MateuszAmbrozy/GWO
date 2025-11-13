## To set environment run:
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt

## To run app:
make && ./tsp_solver

## To visualize results:
python3 visualisation.py
Result will be saved to plot.png file


## Performance
### sudo /usr/bin/perf stat ./tsp_solver
Performance counter stats for './tsp_solver':

          4 417,10 msec task-clock                       #    0,997 CPUs utilized             
               249      context-switches                 #   56,372 /sec                      
                54      cpu-migrations                   #   12,225 /sec                      
               187      page-faults                      #   42,335 /sec                      
    36 084 708 140      cpu_atom/instructions/           #    2,94  insn per cycle              (5,71%)
    47 371 030 831      cpu_core/instructions/           #    3,09  insn per cycle              (93,43%)
    12 268 328 700      cpu_atom/cycles/                 #    2,777 GHz                         (5,74%)
    15 310 800 730      cpu_core/cycles/                 #    3,466 GHz                         (93,43%)
     5 411 338 409      cpu_atom/branches/               #    1,225 G/sec                       (5,74%)
     7 125 473 231      cpu_core/branches/               #    1,613 G/sec                       (93,43%)
         9 789 583      cpu_atom/branch-misses/          #    0,18% of all branches             (5,76%)
        11 550 332      cpu_core/branch-misses/          #    0,16% of all branches             (93,43%)
             TopdownL1 (cpu_core)                 #      2,4 %  tma_backend_bound      
                                                  #      1,6 %  tma_bad_speculation    
                                                  #     35,6 %  tma_frontend_bound     
                                                  #     60,4 %  tma_retiring             (93,43%)
                                                  #      2,1 %  tma_bad_speculation    
                                                  #     63,7 %  tma_retiring             (5,73%)
                                                  #      5,0 %  tma_backend_bound      
                                                  #     29,1 %  tma_frontend_bound       (5,79%)

       4,430533504 seconds time elapsed

       4,416396000 seconds user
       0,001999000 seconds sys

       
### gmon
./tsp_solver && gprof ./tsp_solver gmon.out > raport.txt


### Visualize bottle necks
make && sudo perf record -F 99 -g -- ./tsp_solver && sudo perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
xdg-open flame.svg