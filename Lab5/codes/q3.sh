BUCKET_CAP=500        # The bucket capacity of the token bucket
FIFO_CAP=1000         # The FIFO queue capacity
FIFO_RATE=10.0        # The FIFO queue output data rate
MAX_DECIMAL=6         # Maximum number of decimal places for value of x
PROG_SHAPE=./shape    # The token bucket algorithm program
PROG_FIFO=./fifo      # The fifo queue program
INP_FILE=arrivals.txt # The input file for the token bucket program

# The original number of packets in the input file
ORG_PACK_CNT=$(wc -l <${INP_FILE})

# Function that passes the packets to the token program
# and then pipes the output to the FIFO program
# and return 1 if the output packets are equal to the original number of packets, else 0
function checkAtX {
    x=$1 # The value of x i.e. the token rate for the token program

    # Evaluate the number of output packets after token bucket and fifo queue program
    pack_cnt_x=$(${PROG_SHAPE} ${BUCKET_CAP} ${x} <${INP_FILE} | ${PROG_FIFO} ${FIFO_CAP} ${FIFO_RATE} | wc -l)

    if [[ $(echo "${ORG_PACK_CNT} == ${pack_cnt_x}" | bc -l) -eq 1 ]]; then
        # If output packet are equal to the original number of packets, return 1
        echo 1
    else
        # Else return 0
        echo 0
    fi
}

# The least value possible for x upto MAX_DECIMAL places
low=$(echo "scale=${MAX_DECIMAL}; (1 / 10 ^ ${MAX_DECIMAL})" | bc -l)

# The highest value possible for x will be equal to the output data rate of the FIFO queue
# because for a value greater than that, we will always have packet loss
high=${FIFO_RATE}

# The previous middle value, to check for convergence
prev_mid=${low}

# Search the value of x using binary search
while [[ $(echo "${high} >= ${low}" | bc -l) -eq 1 ]]; do
    # Compute the middle value
    mid=$(echo "scale=${MAX_DECIMAL}; (${low} + ${high}) / 2" | bc -l)

    # If the current middle value is equal to the previous middle value
    # then the convergence has occurred, and we can stop
    if [[ $(echo "${prev_mid} == ${mid}" | bc -l) -eq 1 ]]; then
        break
    fi

    # Check the output at the mid value
    validateMid=$(checkAtX ${mid})

    if [[ ${validateMid} -eq 1 ]]; then
        # If there is no packet loss, then check for the higher values of x
        low=${mid}
    else
        # If there is a packet loss, then check for the lower values of x
        high=${mid}
    fi

    # Update the value of the previous mid
    prev_mid=${mid}
done

# Print the largest value of x found
echo ${prev_mid}
