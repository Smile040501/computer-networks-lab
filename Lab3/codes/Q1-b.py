# 111901030
# Mayank Singla

# %%
from random import random
from queue import Queue
import numpy as np
import matplotlib.pyplot as plt


class User:
    """
    User class to represent each user
    """

    def __init__(self, gen_prob: float, trans_prob: float) -> None:
        """
        Constructor
        """
        if gen_prob < 0 or gen_prob > 1 or trans_prob < 0 or trans_prob > 1:
            raise Exception("probability should be in the interval [0, 1]")

        # The probability of generating a new ethernet frame
        self.gen_prob = gen_prob

        # The probability of transmitting a frame on an idle channel
        self.trans_prob = trans_prob

        # The queue for generated frames
        self.frame_queue = Queue()

    def gen_frame(self) -> int:
        """
        Generates a new ethernet frame with probability `self.gen_prob`
        and put it in the frame_queue.\n
        Returns 1 if new frame is generated, else 0
        """
        # Generate a random number b/w [0, 1) and if that number is less than
        # `self.gen_prob` then a new ethernet frame is generated
        if random() < self.gen_prob:
            self.frame_queue.put(1)  # Enqueueing the generated frame
            return 1
        return 0

    def trans_frame(self) -> int:
        """
        Transmits a frame from the queue with the probability `self.trans_prob`\n
        Returns 1 if it tries to transmit a frame, else 0
        """
        if self.frame_queue.empty():
            # If there is no frame in the queue to be transmitted
            return 0
        elif random() < self.trans_prob:
            # Generate a random number b/w [0, 1) and if that number is less than
            # `self.trans_prob` then a frame is transmitted
            return 1
        return 0

    def trans_succ(self):
        """
        Removes the frame from the frame_queue on successful transmission
        """
        if self.frame_queue.empty():
            raise Exception(
                "Frame Queue should not be empty on successful transmission"
            )

        # Dequeue the frame from the queue
        return self.frame_queue.get()

    def queue_len(self):
        """
        Returns the length of the frame queue
        """
        return len(self.frame_queue.queue)


if __name__ == "__main__":
    NUM_USERS = 100  # Number of users

    NUM_SLOTS = 100  # Number of slots
    SLOT_LEN = 1  # Slot Length (τ)

    FTT = 3 * SLOT_LEN  # Ethernet frame transmission time

    LAMBDA_MIN = 0  # minimum λ value
    LAMBDA_MAX = 1  # maximum λ value
    NUM_LAMBDAS = 100  # number of λ points generated

    # λ points generated uniformly in the interval
    lambda_vals = np.linspace(LAMBDA_MIN, LAMBDA_MAX, NUM_LAMBDAS)

    PROB_VALS = [0.5, 0.01]  # The p-persistence probability values

    avg_tputs_arr = []  # The average throughput for each probability value
    avg_qlens_arr = []  # The average queue length for each probability value

    for p in PROB_VALS:
        avg_tputs = []  # The average throughput for each λ
        avg_qlens = []  # The average queue length for each λ

        # Evaluating average throughput for each λ
        for l in lambda_vals:
            succ_trans = 0  # Number of successful transmission for each λ
            qlen = 0  # Summation of queue length of each user at each slot

            # Generating different users with the probability of generating
            # an ethernet frame as `λ / NUM_USERS` and the probability of
            # transmitting a frame on an idle channel as `p`
            users = [User(l / NUM_USERS, p) for _ in range(NUM_USERS)]

            # Time to wait for transmission
            wait_time = 0

            # Iterating for each slot
            for _ in range(NUM_SLOTS):
                # Recording whether users generated a frame or not
                gen_frames = []
                for u in users:
                    gen_frames.append(u.gen_frame())
                    qlen += u.queue_len()

                if wait_time != 0:
                    # The channel is busy
                    wait_time -= SLOT_LEN
                    continue

                # Transmitted frames. Here the generated frames in the queue
                # are transmitted as per p-persistent CSMA
                trans_frames = [u.trans_frame() for u in users]

                # Transmitted users which actually want to transmit a frame
                trans_users = [x for x in enumerate(trans_frames) if x[1] == 1]

                # Number of users which transmit a frame
                num_users_trans = len(trans_users)

                if num_users_trans == 1:
                    # If only 1 user transmitted a frame, then there is no
                    # collision and we got a successful transmission
                    trans_user_idx = trans_users[0][0]
                    users[trans_user_idx].trans_succ()
                    succ_trans += 1
                    wait_time += FTT
                elif num_users_trans == 0:
                    # If no user transmitted a frame, skip the slot
                    wait_time += SLOT_LEN
                else:
                    # If many users transmitted a frame, then there is a
                    # collision, we wait for the frame transmission time
                    # for the transmission of the corrupted frame
                    wait_time += FTT

                # Decreasing the wait time by 1 after every slot
                wait_time -= SLOT_LEN

            # Calculating the average throughput
            avg_tput = succ_trans / NUM_SLOTS
            avg_tputs.append(avg_tput)

            # Calculating the average queue length
            avg_qlen = qlen / NUM_SLOTS
            avg_qlens.append(avg_qlen)

        # Storing the average throughput and queue length for the probability `p`
        avg_tputs_arr.append(avg_tputs)
        avg_qlens_arr.append(avg_qlens)

    # Plotting the curve for average throughput vs λ
    plt.title("p-persistent CSMA with Queue")
    plt.xlabel("λ (Arrival rate of frames)")
    plt.ylabel("Average Throughput")
    for p, avg_tputs in zip(PROB_VALS, avg_tputs_arr):
        plt.plot(lambda_vals, avg_tputs, label=f"p = {p}")
    plt.grid()
    plt.legend()
    plt.show()

    # Plotting the curve for average queue length vs λ
    plt.title("p-persistent CSMA with Queue")
    plt.xlabel("λ (Arrival rate of frames)")
    plt.ylabel("Average Queue Length")
    for p, avg_qlens in zip(PROB_VALS, avg_qlens_arr):
        plt.plot(lambda_vals, avg_qlens, label=f"p = {p}")
    plt.grid()
    plt.legend()
    plt.show()
