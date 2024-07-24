#!/bin/sh

# make clean
make || exit 1

# check that config is okay.
./config-test || exit 1

BIN="confedit-api"

CGIDIR=/data/cgi-bin
WEBDIR=/data/www
CONFEDITDIR=$WEBDIR/confedit
ETCDIR=/data/etc
LOGDIR=/var/log/confedit
WEBUSER=www-data

if [ -d "$LOGDIR" ] ; then
  echo "$LOGDIR already exists"
else
  sudo mkdir "$LOGDIR"
  sudo chown $WEBUSER.$WEBUSER "$LOGDIR"
  sudo chown 755 "$LOGDIR"
fi

if [ -d "$CGIDIR" ] ; then
  echo "$CGIDIR already exists"
else
  sudo mkdir "$CGIDIR"
  sudo chown root.root "$CGIDIR"
  sudo chown 755 "$CGIDIR"
fi

if [ -d "$WEBDIR" ] ; then
  echo "$WEBDIR already exists"
else
  sudo mkdir "$WEBDIR"
  sudo chown root.root "$WEBDIR"
  sudo chown 755 "$WEBDIR"
fi

if [ -d "$CONFEDITDIR" ] ; then
  echo "$CONFEDITDIR already exists"
else
  sudo mkdir "$CONFEDITDIR"
  sudo chown root.root "$CONFEDITDIR"
  sudo chown 755 "$CONFEDITDIR"
fi

for b in $BIN; do
  if [ -f "$b" ]; then
    if [ -f "$CGIDIR/$b" ]; then
      sudo rm "$CGIDIR/$b"
    fi
    sudo install -o root -g root -m 755 "$b" "$CGIDIR"
  fi
done

if [ -d "$ETCDIR" ] ; then
  echo "$ETCDIR already exists"
else
  sudo mkdir "$ETCDIR"
  sudo chown root.root "$ETCDIR"
  sudo chmod 755 "$ETCDIR"
fi

sudo install -o root -g root -m 644 ui.html $CONFEDITDIR
sudo install -o root -g root -m 644 ui.js $CONFEDITDIR
sudo install -o root -g root -m 644 ui.css $CONFEDITDIR
sudo install -o root -g root -m 644 schema.pdf $CONFEDITDIR
sudo install -o root -g root -m 644 config.ini /usr/local/etc/confedit.ini

