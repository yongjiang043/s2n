#Queue
A queue is buffer with fixed capacity. The elements in the queue follow the FIFO rule. The enqueue process produces a data randomly and puts it into the queue. The dequeue process gets a data from the queue.

##SPIN Model

    #define LENGTH 3
    
    mtype = {p1, p2, p3};
    
    chan queue = [LENGTH] of {mtype};
    
    active proctype enqueue()
    {
       mtype ele;
     
       do   
       :: 
        if
        :: ele = p1;
        :: ele = p2;
        :: ele = p3;
        fi;
        queue!ele;
       od;
    }
    
    active proctype dequeue()
    {
       mtype res;
    
       do
       ::  queue?res;
       od;
    }

##NuSMV Model Generated from S2N

    MODULE main
      VAR
        queue : process Ch_3_mtype;
        p_enqueue : process enqueue(self);
        p_dequeue : process dequeue(self);
      TRANS
        !running
    
    MODULE Ch_3_mtype
      VAR
        ele1 : array 0..2 of { p3, p2, p1 };
        point : -1..2;
      ASSIGN
        init(ele1[0]) := p3;
        init(ele1[1]) := p3;
        init(ele1[2]) := p3;
        init(point) := 2;
      TRANS
        !running
    
    MODULE enqueue(sup)
      VAR
        ele : { p3, p2, p1 };
        pc : 1..7;
      ASSIGN
        init(ele) := p3;
        init(pc) := 1;
        next(ele) := 
                case
                   pc = 3 : p1;
                   pc = 4 : p2;
                   pc = 5 : p3;
                   TRUE   : ele;
                esac;
        next(sup.queue.ele1[0]) := 
                case
                   pc = 6 & sup.queue.point = 0 : ele;
                   TRUE   : sup.queue.ele1[0];
                esac;
        next(sup.queue.ele1[1]) := 
                case
                   pc = 6 & sup.queue.point = 1 : ele;
                   TRUE   : sup.queue.ele1[1];
                esac;
        next(sup.queue.ele1[2]) := 
                case
                   pc = 6 & sup.queue.point = 2 : ele;
                   TRUE   : sup.queue.ele1[2];
                esac;
        next(sup.queue.point) := 
                case
                   pc = 6 & sup.queue.point >= 0 : sup.queue.point - 1;
                   TRUE   : sup.queue.point;
                esac;
      TRANS
          ( running & (
                   pc = 1 & next(pc) = 2
                 | pc = 2 & next(pc) = 3
                 | pc = 2 & next(pc) = 4
                 | pc = 2 & next(pc) = 5
                 | pc = 3 & next(pc) = 6
                 | pc = 4 & next(pc) = 6
                 | pc = 5 & next(pc) = 6
                 | pc = 6 & sup.queue.point >= 0 & next(pc) = 1
                 )
          | !running & next(pc) = pc )
        & ( pc = 2
          | pc = 3
          | pc = 4
          | pc = 5
           -> running )
        & ( pc = 6 & sup.queue.point < 0
          | pc = 7
           -> !running )
    
    
    MODULE dequeue(sup)
      VAR
        res : { p3, p2, p1 };
        pc : 1..3;
      ASSIGN
        init(res) := p3;
        init(pc) := 1;
        next(sup.queue.ele1[0]) := 
                case
                   pc = 2 & sup.queue.point < 2 : p3;
                   TRUE   : sup.queue.ele1[0];
                esac;
        next(sup.queue.ele1[1]) := 
                case
                   pc = 2 & sup.queue.point < 2 : sup.queue.ele1[0];
                   TRUE   : sup.queue.ele1[1];
                esac;
        next(sup.queue.ele1[2]) := 
                case
                   pc = 2 & sup.queue.point < 2 : sup.queue.ele1[1];
                   TRUE   : sup.queue.ele1[2];
                esac;
        next(res) := 
                case
                   pc = 2 & sup.queue.point < 2 : sup.queue.ele1[2];
                   TRUE   : res;
                esac;
        next(sup.queue.point) := 
                case
                   pc = 2 & sup.queue.point < 2 : sup.queue.point + 1;
                   TRUE   : sup.queue.point;
                esac;
      TRANS
          ( running & (
                   pc = 1 & sup.queue.point < 2 & next(pc) = 2
                 | pc = 2 & next(pc) = 1
                 )
          | !running & next(pc) = pc )
        & ( pc = 2
           -> running )
        & ( pc = 1 & sup.queue.point >= 2
          | pc = 3
           -> !running )
