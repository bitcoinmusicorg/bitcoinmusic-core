#!/bin/sh

# Copy Config file if doesn't exist
if [ ! -f "${DATADIR}/config.ini" ]
  then
    cp "/etc/BTCM/config.ini" "${DATADIR}/config.ini"
fi


# Start the node and make sure the blockchain is up to date
exec "/usr/local/bin/btcmd" --data-dir="${DATADIR}" "$@"
