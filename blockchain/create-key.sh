#!/bin/bash

# Create a key pair
keys=$(cleos create key --to-console)
echo "$keys" > passwd

# Get the absolute path of the passwd file
passwd_path=$(readlink -f passwd)

# Print the path and contents of the passwd file
echo "Keys saved to  path: $passwd_path"
cat passwd