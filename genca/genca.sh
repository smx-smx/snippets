#!/bin/bash
## Made by Smx <smxdev4@gmail.com>
## Copyright(C) 2016

## Place this script in /etc/ipsec.d

## Certificate Authority Filename prefix (cacert + cakey)
CA="strongSwanCert"

## Certificate Authority Canonical Name
CA_CN="strongSwan Root CA"

## Lifetime in days of the Certificate Authority
CA_LIFETIME=3650
## Lifetime in days of the issued certificates
CERT_LIFETIME=720


## Generate a private RSA Key
function genKey(){
	local NAME="$1"
	local SIZE=$2

	echo "[+] RSA Key: ${NAME}-${SIZE}"
	ipsec pki \
		--gen \
		--type rsa \
		--size ${SIZE} \
		--outform pem \
	> private/${NAME}.pem

	chmod 600 private/${NAME}.pem
}

## Generate a Certificate Authority
function genCa(){
	# Filename of the new CA
	local NAME="$1"


	echo "[+] CA: ${NAME}"

	ipsec pki \
		--self \
		--ca \
		--lifetime ${CA_LIFETIME} \
		--in private/${NAME}.pem \
		--type rsa \
		--dn "C=CH, O=strongSwan, CN=${CA_CN}" \
		--outform pem \
	> cacerts/${NAME}.pem
}

## Generate a self-signed Certificate
function genCert(){
	# Filename for the new certificate
	local NAME="$1"

	# Canonical Name of the new certificate
	local CN="$2"

	local isServer=$3
	local certFlags
	if [ $isServer -eq 1 ]; then
		certFlags="--flag serverAuth --flag ikeIntermediate"
	fi


	echo "[+] Cert: ${NAME}"

	ipsec pki \
		--pub \
		--in private/${NAME}.pem \
		--type rsa | \
	ipsec pki \
		--issue \
		--lifetime ${CERT_LIFETIME} \
		--cacert cacerts/${CA}.pem \
		--cakey private/${CA}.pem \
		--dn "C=CH, O=strongSwan, CN=${CN}" \
		--san ${CN} \
		${certFlags} \
		--outform pem \
	> certs/${NAME}.pem

}

function genServer(){
	local NAME="$1"
	local CN="$2"

	echo "[+] Server: ${NAME}.pem -> ${CN}"

	genCert "${NAME}" "${CN}" 1
}

function genClient(){
	local NAME="$1"
	local CN="$2"
	#local SAN="$2"

	echo "[+] Client: ${NAME}.pem -> ${CN}"

	genCert "${NAME}" "${CN}" 0

}

function genP12(){
	local NAME="$1"

	if [ ! -d p12 ]; then
		mkdir p12
	fi

	echo "[+] P12: ${NAME}.p12"

	openssl pkcs12 \
		-export \
		-inkey private/${NAME}.pem \
		-in certs/${NAME}.pem \
		-name "${NAME}'s VPN Certificate" \
		-certfile cacerts/${CA}.pem \
		-caname "${CA_CN}" \
		-out p12/${NAME}.p12
}

## Generate a CA if it's missing
function genIfMissing_Ca(){
	local NAME="$1"
	genIfMissing_Key "${NAME}" 4096
	if [ ! -f cacerts/${NAME}.pem ]; then
		genCa "${NAME}"
	fi
}

## Generate a private key if it's missing
function genIfMissing_Key(){
	local NAME="$1"
	local SIZE=$2
	if [ ! -f private/${NAME}.pem ]; then
		genKey "${NAME}" ${SIZE}
	fi
}

## Generate a certificate if it's missing
function genIfMissing_Cert(){
	local NAME="$1"
	local CN="$2"
	local isServer=$3
	if [ ! -f certs/${NAME}.pem ]; then
		genServer "${NAME}" "${CN}" ${isServer}
	fi
}

## Generate a server certificate if it's missing
function genIfMissing_Server(){
	local NAME="$1"
	local CN="$2"
	genIfMissing_Key "${NAME}" 2048
	genIfMissing_Cert "${NAME}" "${CN}" 1
}

## Generate a P12 bundle if it's missing
function genIfMissing_P12(){
	local NAME="$1"
	if [ ! -f p12/${NAME}.p12 ]; then
		genP12 "${NAME}"
	fi
}


## Generate a client certificate if it's missing
function genIfMissing_Client(){
	local NAME="$1"
	local CN="$2"
	genIfMissing_Key "${NAME}" 2048
	genIfMissing_Cert "${NAME}" "${CN}" 0
	genIfMissing_P12 "${NAME}"
}

genIfMissing_Ca "${CA}"

genIfMissing_Server "vpnHost" "example.com"
genIfMissing_Client "User" "foobar@example.com"
genIfMissing_Client "Android" "My Android Phone"
