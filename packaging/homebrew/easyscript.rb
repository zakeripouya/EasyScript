# A draft Homebrew formula for EasyScript. Not published yet: when v0.2.0 is
# tagged, fill in the tarball's sha256 (shasum -a 256 easyscript-0.2.0.tar.gz)
# and put this in a tap (a repository called homebrew-easyscript).
class Easyscript < Formula
  desc "Programming language that reads like English, compiled to C"
  homepage "https://github.com/zakeripouya/EasyScript"
  url "https://github.com/zakeripouya/EasyScript/archive/refs/tags/v0.2.0.tar.gz"
  sha256 "FILL IN WHEN v0.2.0 IS TAGGED"
  license "Apache-2.0" # with a runtime exception (modeled on Swift's) at the end of LICENSE
  head "https://github.com/zakeripouya/EasyScript.git", branch: "master"

  # It compiles programs with the system's C compiler (`cc`), so that stays
  # a runtime need; Xcode's command line tools give it on macOS.

  def install
    system "make"
    system "make", "install", "PREFIX=#{prefix}"
  end

  test do
    assert_match "EasyScript 0.2.0", shell_output("#{bin}/easyscript --version")
    (testpath/"hello.es").write("say \"Hello from Homebrew\".\n")
    assert_equal "Hello from Homebrew\n", shell_output("#{bin}/easyscript run #{testpath}/hello.es")
  end
end
