# 111901030
# Mayank Singla

# %%
from random import random
import numpy as np
import matplotlib.pyplot as plt


class User:
    """
    User class to represent each user
    """

    def __init__(self, gen_prob: float) -> None:
        """
        Constructor
        """
        if gen_prob < 0 or gen_prob > 1:
            raise Exception("probability should be in the interval [0, 1]")

        # The probability of generating a new ethernet frame
        self.gen_prob = gen_prob

    def gen_frame(self) -> int:
        """
        Generates a new ethernet frame with probability `self.gen_prob`\n
        Returns 1 if a new frame is generated, else 0
        """
        # Generate a random number b/w [0, 1) and if that number is less than
        # `self.gen_prob` then a new ethernet frame is generated
        if random() < self.gen_prob:
            return 1
        return 0


if __name__ == "__main__":
    NUM_USERS = 100  # Number of users

    NUM_SLOTS = 100  # Number of slots
    SLOT_LEN = 1  # Slot Length (τ)

    FTT = SLOT_LEN  # Ethernet frame transmission time

    LAMBDA_MIN = 0  # minimum λ value
    LAMBDA_MAX = 1  # maximum λ value
    NUM_LAMBDAS = 100  # number of λ points generated

    # λ points generated uniformly in the interval
    lambda_vals = np.linspace(LAMBDA_MIN, LAMBDA_MAX, NUM_LAMBDAS)

    avg_tputs = []  # The average throughput for each λ
    theo_tputs = []  # The theoretical predictions for each λ

    # Evaluating average throughput for each λ
    for l in lambda_vals:
        succ_trans = 0  # Number of successful transmission for each λ

        # Generating different users with the probability of generating
        # an ethernet frame as `λ / NUM_USERS`
        users = [User(l / NUM_USERS) for _ in range(NUM_USERS)]

        # Time to wait for transmission
        wait_time = 0

        # Iterating for each slot
        for _ in range(NUM_SLOTS):
            # Recording whether users generated a frame or not
            gen_frames = [u.gen_frame() for u in users]

            if wait_time != 0:
                # The channel is busy
                wait_time -= SLOT_LEN
                continue

            # Transmitted users which actually want to transmit a frame
            # Here all the generated frames are transmitted
            trans_users = [x for x in enumerate(gen_frames) if x[1] == 1]

            # Number of users which transmit a frame
            num_users_trans = len(trans_users)

            if num_users_trans == 1:
                # If only 1 user transmitted a frame, then there is no
                # collision and we got a successful transmission
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

        # Calculating the theoretical throughput
        # Throughput, S = (λ/n) * ((1 - (λ / n))^(n-1)) * n     # For one slot, where n = NUM_USERS
        # Avg. S = S * num_slots / num_slots = S
        theo_tput = l * ((1 - (l / NUM_USERS)) ** (NUM_USERS - 1))
        theo_tputs.append(theo_tput)

    # Plotting the curve for average throughput vs λ
    plt.title("Slotted ALOHA")
    plt.xlabel("λ (Arrival rate of frames)")
    plt.ylabel("Average Throughput")
    plt.plot(lambda_vals, avg_tputs, "r", label="Computed")
    plt.plot(lambda_vals, theo_tputs, "g", label="Theoretical")
    plt.grid()
    plt.legend()
    plt.show()
