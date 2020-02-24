#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <err.h>
#include <fcntl.h>
#include <iostream>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

constexpr auto RAM_SIZE = 512000000;
constexpr uint8_t code[] = {
    0xba, 0xf8, 0x03, /* mov $0x3f8, %dx */
    0x00, 0xd8,       /* add %bl, %al */
    0x04, '0',        /* add $'0', %al */
    0xee,             /* out %al, (%dx) */
    0xb0, '\n',       /* mov $'\n', %al */
    0xee,             /* out %al, (%dx) */
    0xf4,             /* hlt */
};

/* KVM API example taken from
* https://lwn.net/Articles/658512/
*
* Goal are to:
*  - Abstract some of this into some sort of C++ API
*  - Add support 64-bit long mode
*  - Then try to load some assembly that reads PMCs
* 
*/
int main(void) {
  int kvm_fd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
  if (kvm_fd == -1) {
    err(1, "/dev/kvm");
  }

  /* Make sure we have the stable version of the KVM API */
  int ret = ioctl(kvm_fd, KVM_GET_API_VERSION, nullptr);
  if (ret == -1) {
    err(1, "KVM_GET_API_VERSION");
  } else if (ret != 12) {
    errx(1, "KVM_GET_API_VERSION %d, expected 12", ret);
  }

  /* Check for required extensions */
  ret = ioctl(kvm_fd, KVM_CHECK_EXTENSION, KVM_CAP_USER_MEMORY);
  if (ret == -1) {
    err(1, "KVM_CHECK_EXTENSION");
  } else if (ret == 0u) {
    errx(1, "Required extension KVM_CAP_USER_MEM not available");
  }


  /* Allocate one aligned page of guest memory to hold the code */
  uint8_t *mem = reinterpret_cast<uint8_t *>(
      mmap(nullptr, RAM_SIZE, PROT_READ | PROT_WRITE,
           MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0));
  if (static_cast<void *>(mem) == MAP_FAILED) {
    err(1, "Allocating guest memory");
  }

  /* Let's create VM */
  int vm_fd = ioctl(kvm_fd, KVM_CREATE_VM, 0ul);
  if (vm_fd < 0) {
    err(1, "KVM_CREATE_VM");
  }

  /* Map it to the second page frame to avoid the real-mode IDT at 0) */
  kvm_userspace_memory_region region = {};
  region.slot = 0;
  region.guest_phys_addr = 0;
  region.memory_size = RAM_SIZE;
  region.userspace_addr = reinterpret_cast<uint64_t>(mem);

  ret = ioctl(vm_fd, KVM_SET_USER_MEMORY_REGION, &region);
  if (ret == -1) {
    err(1, "KVM_SET_USER_MEMORY_REGION");
  }

  /* Load binary */
  int binary_fd = open("test.bin", O_RDONLY);
  if (binary_fd < 0) {
    err(1, "Cannot open binary file");
  }

  ssize_t rd_ret = 0;
  uint8_t *p = mem;

  while (true) {
    rd_ret = read(binary_fd, p, 4096);
    if (rd_ret <= 0) {
      break;
    }
    std::cout << "read size: " << rd_ret << '\n';
    p += rd_ret;
  }
  int vcpu_fd = ioctl(vm_fd, KVM_CREATE_VCPU, 1);
  if (vcpu_fd < 0) {
    err(1, "KVM_CREATE_VCPU");
  }

  /* map the shared kvm_run structure and following data. */
  ret = ioctl(kvm_fd, KVM_GET_VCPU_MMAP_SIZE, nullptr);
  if (ret == -1) {
    err(1, "KVM_GET_VCPU_MMAP_SIZE");
  }

  kvm_run *run;
  std::size_t mmap_size = static_cast<std::size_t>(ret);
  if (mmap_size < sizeof(*run)) {
    errx(1, "KVM_GET_VCPU_MMAP_SIZE unexpectedly small");
  }
  run = reinterpret_cast<kvm_run *>(
      mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu_fd, 0));
  if (run == 0u) {
    err(1, "mmap vcpu");
  }

  /* Initialize CS to point at 0, via a read-modify-write of sregs */
  kvm_sregs sregs;
  ret = ioctl(vcpu_fd, KVM_GET_SREGS, &sregs);
  if (ret == -1) {
    err(1, "KVM_GET_SREGS");
  }
  sregs.cs.base = 0;
  sregs.cs.selector = 0;
  ret = ioctl(vcpu_fd, KVM_SET_SREGS, &sregs);
  if (ret == -1) {
    err(1, "KVM_SET_SREGS");
  }

  kvm_regs regs = {};
  regs.rip = 0x7c00;
  regs.rflags = 0x2;
  regs.rsp = 2 << 20;
  ret = ioctl(vcpu_fd, KVM_SET_REGS, &regs);
  if (ret == -1) {
    err(1, "KVM_SET_REGS");
  }

  while (true) {
    ret = ioctl(vcpu_fd, KVM_RUN, nullptr);
    if (ret == -1) {
      err(1, "KVM_RUN");
    }

    switch (run->exit_reason) {
    case KVM_EXIT_HLT:
      std::cout << "KVM_EXIT_HLT\n";
      return 0;
    case KVM_EXIT_IO:
      std::cout << "out port: " << run->io.port << '\n';
      std::cout << "data: "
                << static_cast<uint64_t>(
                       *(reinterpret_cast<char *>(run) + run->io.data_offset))
                << '\n';
      break;

    case KVM_EXIT_FAIL_ENTRY:
      errx(1, "KVM_EXIT_FAIL_ENTRY: hardware_entry_failure_reason = 0x%llx",
           run->fail_entry.hardware_entry_failure_reason);
    case KVM_EXIT_INTERNAL_ERROR:
      errx(1, "KVM_EXIT_INTERNAL_ERROR: suberror = 0x%x",
           run->internal.suberror);
    default:
      errx(1, "exit_reason = 0x%x", run->exit_reason);
    }
  }

  munmap(mem, sizeof(code));
  close(vm_fd);

  munmap(run, mmap_size);
  close(vcpu_fd);

  close(kvm_fd);
}