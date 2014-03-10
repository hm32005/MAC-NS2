
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashSet;
 
public class Script {
 
  public static void main(String[] args) {
	Script script = new Script();
	script.parse();
  }
 
  public void parse() {
	String traceFile = "simulation.tr";
	BufferedReader br = null;
	String line = "";
	long recv = 0;
	HashSet<String> hs = new HashSet<String>();
	try {
 		
		br = new BufferedReader(new FileReader(traceFile));
		while ((line = br.readLine()) != null) {
			if(line.contains("MAC") && line.contains("cbr") && line.startsWith("r")){
				recv++;
				int start = line.indexOf("-------") + 1;
				hs.add(line.substring(start));
			}
		}
 
	} 
	catch (FileNotFoundException e) {
		e.printStackTrace();
	} 
	catch (IOException e) {
		e.printStackTrace();
	} 
	finally {
		if (br != null) {
			try {
				br.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		
	}
	System.out.println("Copies Recv = " + recv);
	System.out.println("Packets Recv = " + hs.size());

  }
 
}


 

