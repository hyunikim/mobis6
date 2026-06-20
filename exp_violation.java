public class exp_violation {

    public static void main(String[] args) {

        System.out.println("[INFO] ========================================");
        System.out.println("[INFO] EXP01-J VIOLATION DEMONSTRATION");
        System.out.println("[INFO] NULL misuse leads to runtime exception");
        System.out.println("[INFO] ========================================\n");

        scenario1();
        scenario2();
        scenario3();
    }

    // ❌ Scenario 1: Calling equals on null object
    public static void scenario1() {
        System.out.println("[INFO] Scenario 1: Calling equals() on null object");

        String obj = null;
        String target = "test";

        try {
            System.out.println("[DEBUG] Attempting obj.equals(target)");
            boolean result = obj.equals(target);   // ❌ NPE 발생
            System.out.println("[RESULT] Comparison result: " + result);
        } catch (Exception e) {
            System.out.println("[ALERT] Exception occurred!");
            System.out.println("[ALERT] Details: " + e);
        }

        System.out.println();
    }

    // ❌ Scenario 2: Calling split on null string
    public static void scenario2() {
        System.out.println("[INFO] Scenario 2: Calling split() on null string");

        String input = null;

        try {
            System.out.println("[DEBUG] Attempting input.split()");
            String[] tokens = input.split(",");  // ❌ NPE 발생
            System.out.println("[RESULT] Token count: " + tokens.length);
        } catch (Exception e) {
            System.out.println("[ALERT] Exception occurred!");
            System.out.println("[ALERT] Details: " + e);
        }

        System.out.println();
    }

    // ❌ Scenario 3: throw null
    public static void scenario3() {
        System.out.println("[INFO] Scenario 3: Throwing null");

        try {
            System.out.println("[DEBUG] Attempting throw null");
            throw (RuntimeException) null;   // ❌ NPE 발생
        } catch (Exception e) {
            System.out.println("[ALERT] Exception occurred!");
            System.out.println("[ALERT] Details: " + e);
        }

        System.out.println();
    }
}