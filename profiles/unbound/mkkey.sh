# validity period for certificates
DAYS=7200

# size of keys in bits
BITS=1536

# hash algorithm
HASH=sha256

# issuer and subject name for certificates
SERVERNAME=local-unbound
CLIENTNAME=local-unbound-control

# base name for unbound server keys
SVR_BASE=unbound_server

# base name for unbound-control keys
CTL_BASE=unbound_control


openssl genrsa -out $SVR_BASE.key $BITS
openssl genrsa -out $CTL_BASE.key $BITS

cat >request.cfg <<EOF
[req]
default_bits=$BITS
default_md=$HASH
prompt=no
distinguished_name=req_distinguished_name

[req_distinguished_name]
commonName=$SERVERNAME
EOF

openssl req -key $SVR_BASE.key -config request.cfg  -new -x509 -days $DAYS -out $SVR_BASE.pem

# create trusted usage pem
openssl x509 -in $SVR_BASE.pem -addtrust serverAuth -out $SVR_BASE"_trust.pem"

cat >request.cfg <<EOF
[req]
default_bits=$BITS
default_md=$HASH
prompt=no
distinguished_name=req_distinguished_name

[req_distinguished_name]
commonName=$CLIENTNAME
EOF

openssl req -key $CTL_BASE.key -config request.cfg -new | openssl x509 -req -days $DAYS -CA $SVR_BASE"_trust.pem" -CAkey $SVR_BASE.key -CAcreateserial -$HASH -out $CTL_BASE.pem
