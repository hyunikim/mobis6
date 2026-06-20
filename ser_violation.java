import java.io.*;
import java.nio.file.*;
import java.util.*;

public class ser_violation {

    static void line() {
        System.out.println("------------------------------------------------------------");
    }

    static class DangerousPayload implements Serializable {
        private static final long serialVersionUID = 1L;
        String command;

        DangerousPayload(String cmd) {
            this.command = cmd;
        }

        private void readObject(ObjectInputStream ois) throws IOException, ClassNotFoundException {
            ois.defaultReadObject();
            System.out.println("  [ATTACKER CODE EXEC] DangerousPayload.readObject() invoked");
            System.out.println("  [ATTACKER CODE EXEC] Attempting to execute command = \"" + command + "\"");
            Runtime.getRuntime().exec(command); 
            System.out.println("  [ATTACKER CODE EXEC] Runtime.exec(\"" + command + "\") completed -> process spawned");
        }
    }

    static byte[] serialize(Object obj) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        try (ObjectOutputStream oos = new ObjectOutputStream(baos)) {
            oos.writeObject(obj);
        }
        return baos.toByteArray();
    }
    static Object deserializeFromNetwork(byte[] data) throws Exception {
        try (ByteArrayInputStream bais = new ByteArrayInputStream(data);
             ObjectInputStream ois = new ObjectInputStream(bais)) {
            return ois.readObject(); 
        }
    }

    static void scenario_ser12() throws Exception {
        System.out.println("\n=== Scenario 1: Arbitrary Code Execution via Deserialization of Untrusted Data (SER12-J) ===");
        line();
        System.out.println("  [ATTACKER]  Serializes DangerousPayload instead of SafeData and sends it");
        byte[] maliciousBytes = serialize(new DangerousPayload("calc.exe"));
        System.out.println("  [ATTACKER]  Sending tampered data: " + maliciousBytes.length + " bytes");
        System.out.println("  [SERVER RX] Received " + maliciousBytes.length + " bytes (expected SafeData)");
        System.out.println("  [SERVER]    Starting deserialization (no whitelist validation)...");
        // BUG: Actual class of received data is not validated, so DangerousPayload.readObject() gets executed
        Object result = deserializeFromNetwork(maliciousBytes);
        System.out.println("  [SERVER]    Deserialization complete: " + result.getClass().getName());
        System.out.println("  [RESULT]    Server thinks it processed normally, but attacker code has already executed");
    }

    public static void main(String[] args) throws Exception {
        scenario_ser12();
    }
}
