# audiostreamc
    ssh khan555@amber01.cs.purdue.edu
    cd /homes/khan555/24Spring/CS536/lab5
    <!-- audiostreamc kj.au 4096 buffersize targetbuf 128.10.112.136 44444 logfileC -->
    audiostreamc kj.au 4096 4096 4096 128.10.112.136 44444 logfileC
# audiostreams
    ssh khan555@amber02.cs.purdue.edu
    cd /homes/khan555/24Spring/CS536/lab5
    audiostreams 10 5 10 logfileS 128.10.112.136 44444
    ## Helpful Commands
        lsof -i :<port>
        e.g. lsof -i :55550
        kill -9 <PID>

