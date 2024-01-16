# #!/bin/bash

# # Create JSON files for permissions
# cat <<EOF > owner_permission.json
# {
#   "threshold": 2,
#   "keys": [
#     {"key": "EOS7i8bx8XgEyMxC1aEqT1zPpd3uNgrMERC9eqKHyZkPnoeg84f5k", "weight": 1},
#     {"key": "EOS76Gp7Uf84EmbEocbAYvh1RAqwtrS3DEDAPxp7FBpGMMn9ubFNb", "weight": 1},
#     {"key": "EOS6tjapJyMd4NWtjL2A43okSCXpjVzARNNcsW8mCMNhNE5Ld63ww", "weight": 1}
#   ]
# }
# EOF

# cat <<EOF > active_permission.json
# {
#   "threshold": 1,
#   "keys": [
#     {"key": "EOS7i8bx8XgEyMxC1aEqT1zPpd3uNgrMERC9eqKHyZkPnoeg84f5k", "weight": 1},
#     {"key": "EOS76Gp7Uf84EmbEocbAYvh1RAqwtrS3DEDAPxp7FBpGMMn9ubFNb", "weight": 1},
#     {"key": "EOS6tjapJyMd4NWtjL2A43okSCXpjVzARNNcsW8mCMNhNE5Ld63ww", "weight": 1}
#   ]
# }
# EOF

# # Set owner permission
# cleos set account permission found.tmy owner owner_permission.json -p found.tmy@owner

# # Set active permission
# cleos set account permission found.tmy active active_permission.json -p found.tmy@active

# get the password securly
passwd=$(readlink -f passed)

# import the key
cleos import key $passwd

# create an alias so that it connects to pangea mainnet
alias cleos='cleos -u https://pangea-mainnet-api.com'

# open terminal. user can then do cleos commands easily with key and connected to mainnet
/bin/bash