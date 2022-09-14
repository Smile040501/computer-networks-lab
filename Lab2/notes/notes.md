# Useful Commands

- [Useful Commands](#useful-commands)
- [ssh](#ssh)
- [scp](#scp)
- [ftp](#ftp)
- [lftp](#lftp)
- [wget](#wget)
- [nmap](#nmap)
- [Evolution of HTTP](#evolution-of-http)
  - [HTTP/0.9](#http09)
  - [HTTP/1.0](#http10)
  - [HTTP/1.1](#http11)
  - [HTTP/2](#http2)

# ssh

-   **Secure Shell Host**
-   Protocol used to securely connect to a remote server/system
-   Transfers the data in encrypted form between the host and the client
-   Transfers inputs from the client to the host and relays back the output
-   `ssh` runs at TCP/IP port 22

```sh
ssh user_name@host(IP/Domain_name)

ssh user_name@host -p port_number
```

# scp

-   **Secure Copy Protocol**
-   Used to copy file(s) b/w servers in a secure way
-   Allows secure transferring of files in b/w the local host and the remote host or b/w two remote hosts
-   Uses the same authentication and security as it is used in SSH

```sh
scp [-346BCpqrTv] [-c cipher] [-F ssh_config] [-i identity_file] [-P port] source ... target
```

-   The **source** and **target** may be specified as a local pathname, a remote host with optional path in the form `[user@]host:[path]` or a URI in the form `scp://[user@]host[:port][/path]`
-   `-C`: Compression enable. Decrease time if copying a lot of files
-   `-p`: Preserves modification times, access times, and modes from the original file
-   `-q`: Disables the progress meter
-   `-r`: Recursively copy entire directories

# ftp

-   **File Transfer Protocol**
-   Traditional way to transfer files from clients to servers
-   Not only used to transfer data to a remote server, but also to manage that data
-   No encryption involved

```sh
ftp <hostname>

ftp> [command]

ls       # Lists files in server
cd       # change directory in server
cdup     # change to the parent directory
lcd      # change directory on local machine
pwd      # print the current remote working directory
lpwd     # print the current local working directory
mkdir    # creates a new directory within the current remote directory
get      # Downloads file
mget     # Download multiple files
put      # Uploads file
mput     # Uploads multiple files
delete   # remove a file in the current remote directory
rmdir    # remove a directory in the current remote directory
help     # Lists available commands
```

# lftp

-   It is a file transfer program that allows sophisticated FTP, HTTP, and other connections to other hosts
-   Has a built-in mirror that can download or update a whole directory tree
-   There is also a reverse mirror that uploads or updates a directory tree on the server
-   Mirror can also synchronize directories b/w remote servers, using FXP if available

```sh
lftp <hostname>
lftp <hostname>~> user <username>

lftp -u <username> <hostname>
lftp -u <username>,<password> <hostname>

lftp -u <username>,<password> <hostname> -e "set ftp:ssl-allow off"
```

```sh
help

# List all the files in the remote directory
ls

# List all the files in the current local directory
!ls

# Download a remote file to local
get <filename> -o <output_name>

# Upload a file to remote
put <filename> -o <output_name>

# Download the current remote directory to local
mirror <source> <target>

# Download files in parallel
mirror -P

# Upload a directory to remote
mirror -R <source> <target>

# Resume mirror after interruption
mirror -R -c

# Edit a remote file
edit

# Download a torrent file
torrent <Local_Torrent_File/URL/Magnet_link/Hash> -O <target_location>

# Share a file or directory via torrent
# Will provide the magnet URL
torrent --share <file>
```

# wget

-   Non-interactive network downloader
-   Used to download files from the server even when the user has not logged on to the system and can work in the background without hindering the current process

```sh
wget [-bc] [URL]

# Download a webpage
wget [URL]

# Quiet mode and get HTTP response header
wget -q -S [URL]

# Download a file in background
wget -b [URL]

# Resume a partially downloaded file
wget -c [URL]

# Try a given number of times
wget --tries=10 [URL]

# Turn on recursive retrieving
wget -r [URL]

# Reads a document over HTTP and write it to standard output
wget -O- -q <http://localhost/1/download.txt>
```

# nmap

-   **Network Mapper**
-   Tool for network exploration and security auditing
-   Used for:
    -   Real time information of a network
    -   Detailed information of all the IPs activated on your network
    -   Number of ports open in a network
    -   Provide the list of live hosts
    -   Port, OS and Host scanning

```sh
# Scan a system with hostanme and IP address
sudo nmap -v <host/ip_addr>

# Port scan
sudo nmap -p <port> <ip_addr>
sudo nmap -p 8000-9000 <ip_addr>     # Port range
sudo nmap -p 8000,9000 <ip_addr>     # Port list
sudo nmap -p- <ip_addr>              # Scan all ports (1-65535)

# Ping scan: Check if host is up
sudo nmap -sP <ip_addr>

# Scan to detect firewall settings
# Provide if the firewall is active on the host or not
sudo nmap -sA <ip_addr>

# Ask the server for the versions of services it is running
sudo nmap -sV <ip_addr>

# Find hostnames for the given host by completing a DNS query for each one
sudo nmap -sL <ip_addr>

# Scan using TCP protocols
sudo nmap -sT <ip_addr>

# Scan using UDP protocols
sudo nmap -sU <ip_addr>

# Scan the most popular ports
sudo nmap --top-ports <num> <ip_addr>

# `Aggressive`: Let us know the extra information like OS Detection (-O), version detection, script scanning (-sC), and traceroute (-traceroute)
# T4 for faster execution
sudo nmap -A -T4 <domain>

# Discover our Target Hosting service or Identify Additional Targets according to our needs for quickly tracing the path
sudo nmap --trace <domain>

# Never do reverse DNS resolution on the active IP addresses, since it can be slow
sudo nmap -n <domain>

# Always do reverse DNS resolution on the target IP address
sudo nmap -R <domain>

# Scripts
sudo nmap -Pn --script vuln <ip_addr>
sudo nmap -sV --script http-malware-host <ip_addr>
```

# Evolution of HTTP

## HTTP/0.9

**The one-line protocol**

-   Extremely simple
-   Requests consisted of a single line and started with the only possible method `GET` followed by the path to the resource
-   No HTTP headers
-   Only HTML files could be transmitted
-   No status or error codes

```sh
GET /mypage.html
```

## HTTP/1.0

**Building extensibility**

-   Versioning information was sent within each request (`HTTP/1.0` was appended to the `GET` line)
-   New utilities: `Header`, `Versioning`, `Status Code`, `Content-Type`
-   Methods supported: `GET`, `HEAD`, `POST`

## HTTP/1.1

**The standardized protocol**

-   A connection could be reused, which saved time. It is no longer needed to be opened multiple times to display the resources embedded in the single original document
-   Pipelining was added. This allowed a second request to be sent before the answer to the first one was fully transmitted.
-   Chunked responses were also supported
-   Cache control mechanisms were introduced
-   Content negotiation, including language, encoding, and type, was introduced. A client and server could now agree on which content to exchange.
-   `HOST` header: ability to host different domains from the same IP address allowed server collocation
-   Methods supported: `GET`, `HEAD`, `POST`, `PUT`, `DELETE`, `TRACE`, `OPTIONS`

## HTTP/2

**A protocol for greater performance**

-   Binary protocol rather than a text protocol. Can't be read and created manually.
-   Multiplexed protocol. Parallel requests can be made over the same connection
-   Compresses headers
-   Allows a server to populate data in a client cache through **server push**
