#!/bin/bash
. /etc/os-release

apt-get -y install gnupg-curl ca-certificates
if [ "$VERSION_ID" = "8" ]; then
    apt-key adv --fetch-keys https://download.opensuse.org/repositories/home:/scylladb:/scylla-3rdparty-jessie/Debian_8.0/Release.key
elif [ "$VERSION_ID" = "9" ]; then
    apt-key adv --fetch-keys https://download.opensuse.org/repositories/home:/scylladb:/scylla-3rdparty-stretch/Debian_9.0/Release.key
else
    echo "Unsupported distribution."
    exit 1
fi
apt-get update
