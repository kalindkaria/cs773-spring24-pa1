# CS773 Assignment 1

Refer [this](https://docs.google.com/document/d/1a77P4xrjjIW19FLUkyICbp5-0GycmheJxvPIxOFUmTA/edit?usp=sharing) link to understand the problem statement


## Submission Details
| Member Name | Roll Number |
|-------------|-------------|
| Kalind      | 24d0361     |
| Mrityunjay  | 24d0363     |
| Mriganko    | 24m2134     |


## Task 2A

### Aim
The aim is to create a Covert Cache Channel using Flush+Reload attack and send a small text file from sender to receiver.

### Approach
The design have built upon the implementation used by [[1]](#1) and [[2]](#2) with suitable modifications. The two programs work as follows:

1. **Sender**

- The sender program first maps the pre-agreed **shared** file (here `shared_mem.txt`) into the memory and creates a handle for the same.
- The sender opens the **message** file (here `msg.txt`) and reads the contents of the file into a buffer. It then transmits the buffer contents to the receiver using a protocol defined below.
- The protocol works at the granularity of a block level for synchronization purposes.
- During the transmit phase, it sends a pre-defined bit-sequence (in our case, `10101110`) to the receiver to let it know that it has started transmitting the message. This is followed by the actual message transmission.

    - The mechanism for sending one bit (via call to `send_bit` function) is as follows:
        1. Call `cc_sync`: to synchronize with the receiver by polling until the system clock is a multiple of $2^{20}$ cycles (nearly 1ms for 2 GHz clock) with a small jitter of 1536 cycles.
        2. If the bit to send is `1`, then repeatedly flush the cache for the shared memory region for the duration set by the parameter `CHANNEL_DEFAULT_INTERVAL` cycles (`0x8000` in our case, nearly `16 usec`). This allows us to provide redundancy and mitigate stochasticity of hit/miss time by sampling cache accesses multiple times.
        3. If the transmit bit is `0`, then do nothing for the channel interval set above.

2. **Receiver**

- Like the sender, the receiver maps the **shared** file (here `shared_mem.txt`) into memory and creates a handle for the same.
- During the receive phase, it waits for the pre-defined bit-sequence (in our case, `10101110`) to be received from the sender, at which point it knows that the transmission has started. The message is received as a bit-stream, read one bit at a time. THe received data is stored into a message buffer. The null character `\0` is used to identify the end of the message.

    - The mechanism for detecting each bit (via call to `detect_bit` function) is as follows:
        1. Call `cc_sync`: to synchronize with the sender by polling until the system clock is a multiple of $2^{20}$ cycles (nearly 1ms for 2 GHz clock) with a small jitter of 1536 cycles.
        2. It then calls `probe_timing` that gives the access time for a cache line in the shared memory region. If the bit is set to `1` by the sender, there will be a cache miss, and hence high access time. Else, there will be a cache hit, and access time will be less than threshold (set by `CACHE_MISS_LATENCY` cycles based on calibration).
        3. To provide redundancy and stochasticity in access time, the detection of each bit is done repeatedly over an interval of `CHANNEL_DEFAULT_INTERVAL` cycles (`0x8000` in our case, nearly `16 usec`).
        4. If hits exceeeds miss, it is inferrred as bit `0`, else bit `1`.

### Observations

We run the simulations by varying one of the three parameters keeping the other two constant.

- **`CHANNEL_DEFAULT_INTERVAL`**: Controls the time over which cache accesses are done (default `0x8000` cycles or `32768` cycles).
- **`CHANNEL_SYNC_TIMEMASK`**: Controls the time time interval at which the next sync is to be done (periodicity in sync) (default `0xFFFFF` cycles or `1048576` cycles).
- **`CHANNEL_SYNC_JITTER`**: Controls the error margin we tolerate during sync (default `0x600` cycles or `1536` cycles).

An average over 4 iterations is taken for each case.

- Varying `CHANNEL_DEFAULT_INTERVAL`

| Parameter value (hex) | Bandwidth (bps) | Accuracy (%)  |
| :-------------------: | :-------------: | :-----------: |
| 0x2000                |  3244.4         | 99.4          |
| 0x4000                |  3250.2         | 99.7          |
| 0x8000                |  3249.1         | 100.0         |
| 0x10000               |  3246.4         | 99.4          |
| 0x20000               |  3246.7         | 99.7          |

The default interval has on effect on either the bandwidth or the accuracy. The effect on bandwidth is expected as it doesn't effect the rate of tranmission or reception of bits. The most likely reason for independence in accuracy is the fact that all the experimented values provide high redundancy of operation (ranging from 8192 to 131072 clock cycles), thus eliminating stochasticity in cache access time.

- Varying `CHANNEL_SYNC_TIMEMASK`

| Parameter value (hex) | Bandwidth (bps) | Accuracy (%)  |
| :-------------------: | :-------------: | :-----------: |
| 0x3FFFF               |  12987.1        | 99.0          |
| 0x7FFFF               |  6496.1         | 99.7          |
| 0xFFFFF               |  3249.1         | 100.0         |
| 0x1FFFFF              |  1625.2         | 99.4          |
| 0x3FFFFF              |  812.12         | 99.0          |

We see that the accuracy remains fairly independent, but the bandwidth is inversely proportional to the time mask. This is expected because doubling the time mask double the period after which the next sync occurs, and hence the rate becomes half.

- Varying `CHANNEL_SYNC_JITTER`

| Parameter value (hex) | Bandwidth (bps) | Accuracy (%)  |
| :-------------------: | :-------------: | :-----------: |
| 0x200                 |  3246.8         | 36.90         |
| 0x400                 |  3250.2         | 79.76         |
| 0x600                 |  3249.1         | 100.0         |
| 0x800                 |  3247.6         | 100.0         |
| 0x1000                |  3250.3         | 100.0         |

Increasign the jitter value gives better accuracy while preserving the bandwidth. This reason for higher accuracy is that the transmitter and receiver sync up more times when a higher jitter is provided, leading to lower bits missed by the receiver and higher accuracy. Also, achievement of 100% accuracy indicates that most of the errors in previous experiments were due to missing of synchronisation by sender or dropping of bits by the receiver, and not due to flipping of bits due to improper comparison of cache access times. This shows that the threshold and redundancy parameters selected for the system are close to the ideal values.

### Results
- **Accuracy:** 100%
- **Bandwidth:** 3249.1 bps

## Plagiarism Checklist
Your answers should be written in a new line below each bullet.

1. Have you strictly adhered to the submission guidelines?

    Yes

2. Have you received or provided any assistance for this assignment beyond public discussions with peers/TAs?

    No, the assistance we received have been cited in the references.

3. If you answered "yes" to the previous question, please specify what part of code you've taken and from whom you've taken it.

    NA

## References

<a id="1">[1]</a> https://github.com/yshalabi/covert-channel-tutorial

<a id="2">[2]</a> https://github.com/moehajj/Flush-Reload/

