import java.io.*;
import java.net.*;

public class ClientTDConnreuse{

	public static void main(String[] args){


		if(args.length !=2){
			System.err.println("Uso corretto: java ClientTDConnreuse server porta");
			System.exit(1);
		}
		try{
			Socket s= new Socket(args[0], Integer.parseInt(args[1]));
			BufferedReader netIn = new BufferedReader( new InputStreamReader(s.getInputStream(),"UTF-8"));
			BufferedWriter NetOut = new BufferedWriter( new OutputStreamWriter(s.getOutputStream(),"UTF-8"));
			BufferedReader keyboard = new BufferedReader(new InputStreamReader(System.in));

			while(true){
				System.out.println("inserisci username:");
				String username= keyboard.readline();
				System.out.println("inserisci password:");
				String password= keyboard.readline();
				System.out.println("inserisci categoria:");
				String categoria= keyboard.readline();
				netOut.write(username);netOut.newLine();
				netOut.write(password);netOut.newLine();
				netOut.write(categoria);netOut.newLine();
				netOut.flush();
				String line;
				while(true){
					line=netIn.readLine();
					if(Line==null){
						s.close();
						System.exit(0);
					}
					if(Line.equals("---END REQUEST---")){
						break;
					}
					System.out.println(line);
				}
			}
		//s.close();

	}
		catch(Exception e){
			System.err.println(e.getMessage());
			e.printStackTrace();
			System.exit(1);
		}
	}

}
