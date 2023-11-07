./fxc/fxc $1 && {
    ./fxi/fxi out.bin && {
        echo Finished successfully
    } || {
        echo Interpretation failed
    }
} || {
    echo Compilation failed
}
