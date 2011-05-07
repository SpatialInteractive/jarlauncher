Jar Launcher
============
Provides an ant task to take a jar file and convert it into a single file executable that can be
run directly on Windows (.exe) or any Posix-like system like a native executable.

Building
--------
	ant dist
	
jarlauncher-ant.jar will be deposited in build/dist

Using
-----
	<taskdef name="jarexec" classname="net.rcode.buildtools.JarExecTask">
		<classpath>
			<pathelement location="directory/of/jarlauncher-ant.jar"/>
		</classpath>
	</taskdef>
	
	<jarexec target="shell" source="myproject.jar" dest="myposixexecutable"/>
	<chmod perm="a+x" file="myposixexecutable"/>
	
	<jarexec target="exe" source="myproject.jar" dest="mywindowsexecutable.exe"/>
	