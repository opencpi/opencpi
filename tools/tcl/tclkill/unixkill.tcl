package provide kill 1.0

proc kill {pid} {
    exec kill $pid
}
