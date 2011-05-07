#!/bin/sh
thisfile="$0"

# Detect java command
# If the environment variable JAVA_CMD is defined and is a valid executable
# then use that.  Otherwise, fallback and read on
if test -z "$JAVA_CMD" || ! test -x "$JAVA_CMD" || ! test -f "$JAVA_CMD"
then
	# Detect JAVA_CMD based on JAVA_HOME if set
	if test -n "$JAVA_HOME" && test -d "$JAVA_HOME"
	then
		JAVA_CMD="$JAVA_HOME/bin/java"
	else
		# Just use "java" and let the system figure it out
		JAVA_CMD="java"
	fi
fi

# Setup Java args.  Always pass the system property exec.file.
if test -z "$JAVA_ARGS"
then
	JAVA_ARGS=""
fi

exec $JAVA_CMD "-Dexec.file=$thisfile" $JAVA_ARGS -jar "$thisfile" "$@"
echo "ERROR: Could not execute JVM"
exit 1
#### Jar contents follow
