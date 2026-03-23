// Forward declare to avoid header path issues during initial wiring
extern void runMinimalDemo();
int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  runMinimalDemo();
  return 0;
}
