# Hydra
This program determines the decision tree that maximizes immediate perfect clear (PC) chance. It does this by executing a brute force search across every possible decision tree.

## Limitations

Hydra's speed arises from the use of a pre-generated graph of all 4-line PC-able fields starting from a blank field. As such, it is unable to work with fields that cannot be generated from a blank field by placing pieces or cannot PC with any sequence of pieces.

## Usage

Download `graph.bin` from [here](https://drive.google.com/file/d/1XEYrDFhatN-McOcpTpWuAYq8ubyanJtq/view?usp=sharing) (Google Drive link). The file should be 510917451 bytes and have the following hashes:

    MD5: 1490c9bd252d6bfa5c7d00518737be6b
    SHA1: 3add9a11996e386db2482763344eb5062a745afa

Compile `hydra_solver.cpp` with your C++ compiler of choice.

    g++ -O3 -std=c++14 hydra_solver.cpp

Invoking the resulting executable should lead to output similar to the following. Warning: by design, almost all Hydra output is located on `stderr`.

    Hydra v0.4.20240203
    Loading graph (took 3672 ms)
    Starting from field 0 (hash 0)
    Running see 7

    >

Enter a string of 7 pieces from `IJLOSTZ` to serve as the queue, then a space, then a string of pieces from `IJLOSTZ` to serve as the partial bag. You should receive output similar to the following.

    > IJLOSTZ IJLOSTZ
    [0] Testing queue IJLOSTZ with bag IJLOSTZ
    Result: 838/840
    Time: 323 ms

    >

Once finished, you will receive another prompt to send more queries. To end the program gracefully, enter a string of length not 7.

Instead of entering a bag, you can also enter a single digit from 1 through 7 representing the size of the bag. The program will infer the bag composition from the queue.

    > OTJLISO 1
    [0] Testing queue OTJLISO with bag Z
    Result: 206/210
    Time: 469 ms

## Customization

The following command line arguments are supported. Combining multiple flags into one argument (e.g. `-bo`) is not supported.

* `-b`: Boolean mode. Returns either 0/1 or 1/1 depending on whether the PC is 100%. This speeds up all cases where at least one piece is hidden from the player at the start, at the downside of returning less precise information if the PC is not 100%. Incompatible with decision mode and weighted mode.
* `-d`: Decision mode. Outputs an optimal decision tree to `tree_data.js`, which can be viewed by opening `tree_viewer.html`. This can be considerably slower than the regular mode. Incompatible with boolean mode and two-line mode.
* `-f [number]`: Starting field hash. Defaults to 0 (the empty field). Must be a 40-bit unsigned integer, and the field it represents must be found in the graph. The hash is explained in detail below. This can also be used as a command while the program is running to change the starting field.
* `-m [number]`: Maximum number of threads. Defaults to `std::thread::hardware_concurrency()`. The argument is capped below by 1 and above by hardware concurrency. This can also be used as a command while the program is running to change the maximum number of threads.
* `-o`: Stdout mode. Echoes all numerical results to stdout. This is useful when running queues in batch and redirecting the raw results to a file.
* `-s [number]`: See (held + active + previews). Defaults to 7. This is the length of the first string given as input. Must be an integer in the range $[2, 11]$. This can also be used as a command while the program is running to change the see.
* `-t`: Two-line mode. Considers 2-line PCs to be successful. When combined with weighted mode, 2-line PCs are always given a weight of 0. Incompatible with decision mode.
* `-w`: Weighted mode. Weights composition-based saves with the weights provided in `weights.txt`. Weights must be integers in the range $[0, 2^{32}]$. The return value is the sum of the weights for all saves across the decision tree, where fail queues are given a weight of $2^{32}$, *minus the sum of minimum weights in the row for each applicable save scenario*; e.g. a return value of 0 indicates that the minimum possible weight is always achieved. Incompatible with boolean mode.

## Field hash

To find the hash of a field, first place all cleared lines at the bottom of the stack. Then the hash is simply the field read as a 40-bit binary number from left to right from top to bottom, where an empty mino represents 0 and a filled mino represents 1. Example:

    xxxxxx....
    xxxxx.....
    xxxxxxxxxx
    xxxxxx...x

Bring the cleared line to the bottom of the stack:

    xxxxxx....
    xxxxx.....
    xxxxxx...x
    xxxxxxxxxx

Now read it as a binary number:

    1111110000
    1111100000
    1111110001
    1111111111

    0b1111110000_1111100000_1111110001_1111111111 = 1083372980223

## Determinism

The first piece placement is subject to a multithreaded branch search. This poses a problem in decision mode if there is more than one optimal strategy, as the one which returns first is non-deterministic. At the cost of a little bit of speed, it is guaranteed that ties are broken in favor of the lexicographically earlier strategy, which is the same tiebreaking method applied in a singlethreaded search. In other words, Hydra is guaranteed to be deterministic.
