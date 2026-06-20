import java.io.BufferedReader;
import java.io.InputStreamReader;

public class cwe78_violation {

    public static void main(String[] args) {

        System.out.println("[INFO] ========================================");
        System.out.println("[INFO] CWE-78 VIOLATION DEMONSTRATION");
        System.out.println("[INFO] OS Command Injection Example");
        System.out.println("[INFO] ========================================\n");

        scenario_command_injection();
    }

    // Vulnerable scenario: command execution without validation
    public static void scenario_command_injection() {

        System.out.println("[INFO] Scenario: OS Command Injection");

        try {
            // Attacker-controlled input (for demo purpose)
            String userInput = "dir  & echo INJECTED_SUCCESS";  

            System.out.println("[DEBUG] User input: " + userInput);

            // ❌ Vulnerable: directly use input in command
            String command = "cmd.exe /c " + userInput;

            System.out.println("[DEBUG] Executing command: " + command);

            Process process = Runtime.getRuntime().exec(command);

            BufferedReader reader =
                    new BufferedReader(new InputStreamReader(process.getInputStream(), "MS949"));

            String line;

            System.out.println("[RESULT] Command output:");
            while ((line = reader.readLine()) != null) {
                System.out.println(line);
            }

            System.out.println("[ALERT] External input executed without validation!");

        } catch (Exception e) {
            System.out.println("[ALERT] Exception occurred!");
            System.out.println("[ALERT] Details: " + e);
        }

        System.out.println();
    }
}
