- [The Medium Access Control SubLayer](#the-medium-access-control-sublayer)
  - [The Channel Allocation Problem](#the-channel-allocation-problem)
    - [Static Channel Allocation](#static-channel-allocation)
    - [Assumptions for Dynamic Channel Allocation](#assumptions-for-dynamic-channel-allocation)
  - [Multiple Access Protocols](#multiple-access-protocols)
    - [ALOHA](#aloha)
      - [Pure ALOHA](#pure-aloha)
      - [Slotted ALOHA](#slotted-aloha)
    - [Carrier Sense Multiple Access](#carrier-sense-multiple-access)
      - [Persistent and Nonpersistent CSMA](#persistent-and-nonpersistent-csma)

# The Medium Access Control SubLayer

-   In any broadcast network, the key issue is how to determine who gets to use the channel when there is competition for it
-   Broadcast channels are sometimes referred to as **Multi-access channels** or **random access channels**
-   The protocols used to determine who goes next on a multi-access channel belong to a sublayer of the data link layer called the **MAC (Medium Access Control)** sublayer

## The Channel Allocation Problem

### Static Channel Allocation

-   Traditional way is to chop up the the capacity by using one of the multiplexing schemes, such as FDM
-   A wireless example is FM radio stations
-   **Disadvantages**
    -   If the band is divided into N regions, there could be less or more than N users
-   Poor performance of static FDM: using simple queueing theory calculation

```sh
T : Mean Time Delay to send a frame onto a channel
C : capacity of the channel (bps)
λ : Arrival rate of frames (frames/sec)
Average length of frames : 1/μ bits

Service rate of the channel is μC frames/sec

T =  1 / (μC - λ)
```

```sh
Divide single channels into N independent subchannels,
each with capacity C/N bps

Mean input rate on each subchannel = λ/N

Tₙ = 1 / μ(C/N) - (λ/N)
  = N / (μC - λ) = NT
```

-   The mean delay of the divided channels is N times worse than if all the frames were somehow magically arranged orderly in a big central queue

### Assumptions for Dynamic Channel Allocation

-   **Independent Traffic**:
    -   The model consists of `N` independent **stations** each with a program/user that generates frames for transmission
    -   Expected no. of frames generated in an interval of length `Δt` is `λΔt`, where `λ` is constant (arrival rate of new frames)
    -   Once a frame is generated, the station is blocked, and does nothing until the frame has been successfully transmitted
-   **Single Channel**:
    -   A single channel is available for all communication
    -   All stations can transmit on it and all can receive from it
    -   The stations are assumed to be equally capable
-   **Observable Collisions**:
    -   If two frames are transmitted simultaneously, they overlap in time and the resulting signal is garbled. This event is called a **collision**
    -   All stations can detect that a collision has occurred
    -   A collided frame must be transmitted again later
    -   No errors other than those generated by collisions occur
-   **Continuous or Slotted Time**:
    -   Time may be assumed continuous, in which case the frame transmission can begin at any instant
    -   Alternatively, time may be slotted or divided into discrete intervals (called slots). Frame transmissions must then begin at the start of a slot. A slot may contain 0, 1, or more frames, corresponding to an idle slot, a successful transmission, or a collision, respectively.
-   **Carrier Sense or No Carrier Sense**:
    -   Stations can tell if the channel is in use before trying to use it
    -   No station will attempt to use the channel while it is sensed as busy
    -   If there is no carrier sense, stations cannot sense the channel before trying to use it. They just go ahead and transmit. Only later can they determine whether the transmission was successful.

## Multiple Access Protocols

### ALOHA

#### Pure ALOHA

-   Continuous time
-   Let users transmit whenever they have data to send
-   Colliding frames will be damaged
-   In the ALOHA system, after each station has sent its frame to the central computer, this computer rebroadcasts the frame to all of the stations. A sending station can thus listen for the broadcast from the hub to see if its frame has gotten through.
-   If the frame was destroyed, the sender just waits a random amount of time and sends it again.
-   Systems in which multiple users share a common channel in a way that can lead to conflicts are known as **contention** systems
-   **Frame time**: amount of time needed to transmit the standard, fixed-length frame (i.e. the frame length divided by the bit rate)
    -   **Vulnerable Time** `= 2 * Frame Time`
-   Assume new frames generated by the stations are well modeled by a Poisson distribution with a mean of `N` frames per frame time
    -   If `N > 1`, the user community is generating frames at a higher rate than the channel can handle, and nearly every frame will suffer a collision
    -   For reasonable throughput, we would expect `0 < N < 1`
-   In addition to the new frames, the stations also generate retransmissions of frames that previously suffered collisions. Let us further assume that the old and new frames combined are well modeled by a Poisson distribution, with mean of `G` frames per frame time
    -   At low load (i.e., `N ≈ 0`), there will be few collisions, hence few retransmissions, so `G ≈ N`
    -   At high load, there will be many collisions, so `G > N`
-   Throughput `S = GP₀`, where P0 is the probability that a frame does not suffer a collision
-   The probability that `k` frames are generated during a given frame time, in which `G` frames are expected, is given by the Poisson distribution
    ```sh
    Pr[k] = Gᵏe⁻ᴳ / k!
    ```
    -   Probability of zero frames is `e⁻ᴳ`
    -   Mean frames generated is `2G`
    -   Probability that no frame is generated during the entire vulnerable period is `e⁻²ᴳ`
    -   `S = Ge⁻²ᴳ`

#### Slotted ALOHA

-   Doubles the capacity of the ALOHA system
-   Divide the time into discrete intervals called **slots**, each interval corresponding to one frame
-   A station is not permitted to send whenever the user types a line. Instead, it is required to wait for the beginning of the next slot.
    -   This halves the vulnerable period
    -   Probability that no frame is generated during the entire vulnerable period is `e⁻ᴳ`
    -   `S = Ge⁻ᴳ`
    -   Probability of a collision is `1 - e⁻ᴳ`
    -   Probability of a transmission requiring exactly k attempts is `Pₖ = e⁻ᴳ(1 - e⁻ᴳ)ᵏ⁻¹`
    -   Expected number of transmissions, `E`, per line typed at a terminal is
        ```sh
        E = Σ k . Pₖ
          = eᴳ
        ```

### Carrier Sense Multiple Access

-   Protocols in which stations listen for a carrier (i.e., a transmission) and act accordingly are called **carrier sense protocols**

#### Persistent and Nonpersistent CSMA

-   Station waits until the channel is idle to send its data
-   **1-persistent CSMA**: Station transmits with a probability of 1 when it finds the channel idle
-   **Non-persistent CSMA**: Upon finding an idle channel, wait for a random amount of time and then repeat the algorithm
-   **p-persistent CSMA**: Station transmits with a probability of `p` when it finds the channel idle