
import java.io.ByteArrayOutputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.Serializable;
import java.util.HashMap;

class parent implements Serializable {
	int parentVersion = 10;
}

class contain implements Serializable{
	int containVersion = 11;
    SerialTest test;

}


class simple_class implements Serializable {
    int num = 9;
}

class advanced_class implements Serializable {
    simple_class simple = new simple_class();
    simple_class empty;
    int num = 8;
}

class large_class implements Serializable {
    advanced_class advanced = new advanced_class();
    String message = "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";
    int num = 9;
    double dec = 0.383;
    HashMap<String, String> dictionary = new HashMap<String, String>();

    {
        dictionary.put("1", "1");
        dictionary.put("2", "2");
    }
}


public class SerialTest extends parent implements Serializable {
	int version = 66;
	contain con = new contain();

	public int getVersion() {
		return version;
	}
	public static void serialize(Object o) throws IOException {
	    ByteArrayOutputStream bos = new ByteArrayOutputStream();
        ObjectOutputStream oos = new ObjectOutputStream(bos);
        oos.writeObject(o);
        oos.flush();
        oos.close();
	}

	public static void timed(String testName, Object o) throws IOException {
        final long startTime = System.currentTimeMillis();


        for (int i = 0; i < 10000000; i++) {
            serialize(o);
        }
        final long endTime = System.currentTimeMillis();

        System.out.println( testName + ": " + (endTime - startTime));
	}

	public static void main(String args[]) throws IOException {
        simple_class simple = new simple_class();
        timed("simple class", simple);

//         for (int i = 0; i < 10000000; i++) {
//             SerialTest st = new SerialTest();
//             st.con.test = st;
//             serialize(st);
//
//             // ByteArrayInputStream bis = new ByteArrayInputStream(bos.toByteArray());
//             // ObjectInputStream in = new ObjectInputStream(bis);
//             // try {
//             // Object copied = in.readObject();
//             // } catch(Exception e){}
//         }

//         System.out.println("Total execution time: " + (endTime - startTime));
	}
}