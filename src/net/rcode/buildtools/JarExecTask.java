package net.rcode.buildtools;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

/**
 * Outputs various artifacts for making a jar file executable on a given
 * platform.  The most direct method of doing this is to add a shell script
 * prefix to an existing jar file.  Since the zip standard requires that
 * any unrecognized patterns before a zip header be skipped, this allows
 * us to create a single file executable that works on any unix-y platform.
 * <p>
 * Windows will still need a companion .cmd script but this cannot be avoided.
 * When I get a chance, I may create a custom exe stub that does the same
 * thing for Windows, but not now.
 * 
 * @author stella
 *
 */
public class JarExecTask extends Task {
	private File source;
	private File dest;
	private String target;
	
	public String getTarget() {
		return target;
	}
	
	public void setTarget(String target) {
		this.target=target;
	}

	public File getSource() {
		return source;
	}
	public void setSource(File source) {
		this.source = source;
	}
	
	public File getDest() {
		return dest;
	}
	public void setDest(File dest) {
		this.dest = dest;
	}
	
	@Override
	public void execute() throws BuildException {
		if (source==null || !source.isFile()) {
			throw new BuildException("Expected 'source' file");
		}
		if (dest==null) {
			throw new BuildException("Expected 'dest' attribute");
		}
		
		log("Creating self executor: " + dest);
		try {
			FileOutputStream out=new FileOutputStream(dest);
			try {
				InputStream header=openHeader();
				copyStream(header, out);
				
				InputStream jarIn=new FileInputStream(source);
				copyStream(jarIn, out);
			} finally {
				out.close();
			}
			
			makeExecutable(dest);
		} catch (IOException e) {
			throw new BuildException("IO Error while creating " + dest, e);
		}
	}
	
	/**
	 * Make the given file executable.  If this is not supported on the platform,
	 * silently does nothing.
	 * @param f
	 */
	private void makeExecutable(File f) {
		f.setExecutable(true, false);
	}
	
	/**
	 * Opens the appropriate header stream
	 * @return header InputStream
	 */
	private InputStream openHeader() throws BuildException {
		if ("exe".equals(target)) {
			return getClass().getResourceAsStream("winlauncher.exe");
		} else if ("shell".equals(target)) {
			return getClass().getResourceAsStream("exec-script-header.sh");
		} else {
			throw new BuildException("Unrecognized value '" + target + "' for target attribute");
		}
	}
	
	/**
	 * Copy input to output.  Close input on finish.
	 * @param in
	 * @param out
	 * @throws IOException 
	 */
	private static void copyStream(InputStream in, OutputStream out) throws IOException {
		try {
			byte[] buffer=new byte[4096];
			for (;;) {
				int r=in.read(buffer);
				if (r<0) break;
				out.write(buffer, 0, r);
			}
		} finally {
			in.close();
		}
	}
}
