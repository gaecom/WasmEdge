// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/defines.h"
#include "common/span.h"
#include "host/wasi/error.h"
#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
#include <dirent.h>
#include <sys/stat.h>

#include <boost/align/aligned_allocator.hpp>
#endif

namespace WasmEdge {
namespace Host {
namespace WASI {

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
struct FdHolder {
  FdHolder(const FdHolder &) = delete;
  FdHolder &operator=(const FdHolder &) = delete;
  FdHolder(FdHolder &&RHS) noexcept : Fd(std::exchange(RHS.Fd, -1)) {}
  FdHolder &operator=(FdHolder &&RHS) noexcept {
    using std::swap;
    swap(Fd, RHS.Fd);
    return *this;
  }

  constexpr FdHolder() noexcept = default;
  ~FdHolder() noexcept { reset(); }
  explicit constexpr FdHolder(int Fd) noexcept : Fd(Fd) {}
  constexpr bool ok() const noexcept { return Fd >= 0; }
  void reset() noexcept;
  int release() noexcept { return std::exchange(Fd, -1); }
  void emplace(int NewFd) noexcept {
    reset();
    Fd = NewFd;
  }
  int Fd = -1;
};

struct DirHolder {
  DirHolder(const DirHolder &) = delete;
  DirHolder &operator=(const DirHolder &) = delete;
  DirHolder(DirHolder &&RHS) noexcept {
    using std::swap;
    swap(Dir, RHS.Dir);
    swap(Cookie, RHS.Cookie);
  }
  DirHolder &operator=(DirHolder &&RHS) noexcept {
    using std::swap;
    swap(Dir, RHS.Dir);
    swap(Cookie, RHS.Cookie);
    return *this;
  }

  DirHolder() noexcept = default;
  explicit DirHolder(DIR *D) noexcept : Dir(D) {}
  constexpr bool ok() const noexcept { return Dir != nullptr; }
  ~DirHolder() noexcept { reset(); }
  void reset() noexcept;
  void emplace(DIR *NewDir) noexcept {
    reset();
    Dir = NewDir;
  }

  DIR *Dir = nullptr;
  uint64_t Cookie = 0;
  std::vector<uint8_t, boost::alignment::aligned_allocator<
                           uint8_t, alignof(__wasi_dirent_t)>>
      Buffer;
};
#endif

#if WASMEDGE_OS_LINUX
struct TimerHolder {
  TimerHolder(const TimerHolder &) = delete;
  TimerHolder &operator=(const TimerHolder &) = delete;
  TimerHolder(TimerHolder &&RHS) noexcept {
    using std::swap;
    swap(Id, RHS.Id);
  }
  TimerHolder &operator=(TimerHolder &&RHS) noexcept {
    using std::swap;
    swap(Id, RHS.Id);
    return *this;
  }

  constexpr TimerHolder() = default;
  explicit constexpr TimerHolder(timer_t T) noexcept : Id(T) {}
  ~TimerHolder() noexcept { reset(); }
  void reset() noexcept;
  void emplace(timer_t NewId) noexcept {
    reset();
    Id = NewId;
  }
  std::optional<timer_t> Id;
};
#endif

class Poller;

class INode
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
    : public FdHolder
#endif
{
public:
  INode(const INode &) = delete;
  INode &operator=(const INode &) = delete;
  INode(INode &&RHS) noexcept = default;
  INode &operator=(INode &&RHS) noexcept = default;

  static INode stdIn() noexcept;

  static INode stdOut() noexcept;

  static INode stdErr() noexcept;

  /// Open a file or directory.
  ///
  /// @param[in] Path The absolut path of the file or directory to open.
  /// @param[in] OpenFlags The method by which to open the file.
  /// @param[in] FdFlags The method by which to open the file.
  /// @param[in] VFSFlags The method by which to open the file.
  /// @return The file descriptor of the file that has been opened, or WASI
  /// error.
  static WasiExpect<INode> open(std::string Path, __wasi_oflags_t OpenFlags,
                                __wasi_fdflags_t FdFlags,
                                uint8_t VFSFlags) noexcept;

  /// Provide file advisory information on a file descriptor.
  ///
  /// Note: This is similar to `posix_fadvise` in POSIX.
  ///
  /// @param[in] Offset The offset within the file to which the advisory
  /// applies.
  /// @param[in] Len The length of the region to which the advisory applies.
  /// @param[in] Advice The advice.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAdvise(__wasi_filesize_t Offset, __wasi_filesize_t Len,
                            __wasi_advice_t Advice) const noexcept;

  /// Force the allocation of space in a file.
  ///
  /// Note: This is similar to `posix_fallocate` in POSIX.
  ///
  /// @param[in] Offset The offset at which to start the allocation.
  /// @param[in] Len The length of the area that is allocated.
  /// @return Nothing or WASI error
  WasiExpect<void> fdAllocate(__wasi_filesize_t Offset,
                              __wasi_filesize_t Len) const noexcept;

  /// Synchronize the data of a file to disk.
  ///
  /// Note: This is similar to `fdatasync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdDatasync() const noexcept;

  /// Get the attributes of a file descriptor.
  ///
  /// Note: This returns similar flags to `fsync(fd, F_GETFL)` in POSIX, as well
  ///
  /// as additional fields.
  /// @param[out] FdStat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatGet(__wasi_fdstat_t &FdStat) const noexcept;

  /// Adjust the flags associated with a file descriptor.
  ///
  /// Note: This is similar to `fcntl(fd, F_SETFL, flags)` in POSIX.
  ///
  /// @param[in] FdFlags The desired values of the file descriptor flags.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFdstatSetFlags(__wasi_fdflags_t FdFlags) const noexcept;

  /// Return the attributes of an open file.
  ///
  /// @param[out] Filestat Result.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatGet(__wasi_filestat_t &Filestat) const noexcept;

  /// Adjust the size of an open file. If this increases the file's size, the
  /// extra bytes are filled with zeros.
  ///
  /// Note: This is similar to `ftruncate` in POSIX.
  ///
  /// @param[in] Size The desired file size.
  /// @return Nothing or WASI error
  WasiExpect<void> fdFilestatSetSize(__wasi_filesize_t Size) const noexcept;

  /// Adjust the timestamps of an open file or directory.
  ///
  /// Note: This is similar to `futimens` in POSIX.
  ///
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  WasiExpect<void>
  fdFilestatSetTimes(__wasi_timestamp_t ATim, __wasi_timestamp_t MTim,
                     __wasi_fstflags_t FstFlags) const noexcept;

  /// Read from a file descriptor, without using and updating the file
  /// descriptor's offset.
  ///
  /// Note: This is similar to `preadv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors in which to store data.
  /// @param[in] Offset The offset within the file at which to read.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPread(Span<Span<uint8_t>> IOVs, __wasi_filesize_t Offset,
                           __wasi_size_t &NRead) const noexcept;

  /// Write to a file descriptor, without using and updating the file
  /// descriptor's offset.
  ///
  /// Note: This is similar to `pwritev` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors from which to retrieve
  /// data.
  /// @param[in] Offset The offset within the file at which to write.
  /// @param[out] NWritten The number of bytes written.
  /// @return Nothing or WASI error
  WasiExpect<void> fdPwrite(Span<Span<const uint8_t>> IOVs,
                            __wasi_filesize_t Offset,
                            __wasi_size_t &NWritten) const noexcept;

  /// Read from a file descriptor.
  ///
  /// Note: This is similar to `readv` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors to which to store data.
  /// @param[out] NRead The number of bytes read.
  /// @return Nothing or WASI error
  WasiExpect<void> fdRead(Span<Span<uint8_t>> IOVs,
                          __wasi_size_t &NRead) const noexcept;

  /// Read directory entries from a directory.
  ///
  /// When successful, the contents of the output buffer consist of a sequence
  /// of directory entries. Each directory entry consists of a `dirent` object,
  /// followed by `dirent::d_namlen` bytes holding the name of the directory
  /// entry.
  ///
  /// This function fills the output buffer as much as possible,
  /// potentially truncating the last directory entry. This allows the caller to
  /// grow its read buffer size in case it's too small to fit a single large
  /// directory entry, or skip the oversized directory entry.
  ///
  /// @param[out] Buffer The buffer where directory entries are stored.
  /// @param[in] Cookie The location within the directory to start reading
  /// @param[out] Size The number of bytes stored in the read buffer. If less
  /// than the size of the read buffer, the end of the directory has been
  /// reached.
  /// @return Nothing or WASI error
  WasiExpect<void> fdReaddir(Span<uint8_t> Buffer, __wasi_dircookie_t Cookie,
                             __wasi_size_t &Size) noexcept;

  /// Move the offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek` in POSIX.
  ///
  /// @param[in] Offset The number of bytes to move.
  /// @param[in] Whence The base from which the offset is relative.
  /// @param[out] Size The new offset of the file descriptor, relative to the
  /// start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdSeek(__wasi_filedelta_t Offset, __wasi_whence_t Whence,
                          __wasi_filesize_t &Size) const noexcept;

  /// Synchronize the data and metadata of a file to disk.
  ///
  /// Note: This is similar to `fsync` in POSIX.
  ///
  /// @return Nothing or WASI error
  WasiExpect<void> fdSync() const noexcept;

  /// Return the current offset of a file descriptor.
  ///
  /// Note: This is similar to `lseek(fd, 0, SEEK_CUR)` in POSIX.
  ///
  /// @param[out] Size The current offset of the file descriptor, relative to
  /// the start of the file.
  /// @return Nothing or WASI error
  WasiExpect<void> fdTell(__wasi_filesize_t &Size) const noexcept;

  /// Write to a file descriptor.
  ///
  /// Note: This is similar to `writev` in POSIX.
  ///
  /// @param[in] IOVs List of scatter/gather vectors from which to retrieve
  /// data.
  /// @param[out] NWritten The number of bytes written.
  /// @return Nothing or WASI error
  WasiExpect<void> fdWrite(Span<Span<const uint8_t>> IOVs,
                           __wasi_size_t &NWritten) const noexcept;

  /// Create a directory.
  ///
  /// Note: This is similar to `mkdirat` in POSIX.
  ///
  /// @param[in] Path The path at which to create the directory.
  /// @return Nothing or WASI error
  WasiExpect<void> pathCreateDirectory(std::string Path) const noexcept;

  /// Return the attributes of a file or directory.
  ///
  /// Note: This is similar to `stat` in POSIX.
  ///
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[out] Filestat The buffer where the file's attributes are stored.
  /// @return Nothing or WASI error
  WasiExpect<void> pathFilestatGet(std::string Path,
                                   __wasi_filestat_t &Filestat) const noexcept;

  /// Adjust the timestamps of a file or directory.
  ///
  /// Note: This is similar to `utimensat` in POSIX.
  ///
  /// @param[in] Path The path of the file or directory to inspect.
  /// @param[in] ATim The desired values of the data access timestamp.
  /// @param[in] MTim The desired values of the data modification timestamp.
  /// @param[in] FstFlags A bitmask indicating which timestamps to adjust.
  /// @return Nothing or WASI error
  WasiExpect<void>
  pathFilestatSetTimes(std::string Path, __wasi_timestamp_t ATim,
                       __wasi_timestamp_t MTim,
                       __wasi_fstflags_t FstFlags) const noexcept;

  /// Create a hard link.
  ///
  /// Note: This is similar to `linkat` in POSIX.
  ///
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path from which to link.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path at which to create the hard link.
  /// resolved.
  /// @return Nothing or WASI error
  static WasiExpect<void> pathLink(const INode &Old, std::string OldPath,
                                   const INode &New,
                                   std::string NewPath) noexcept;

  /// Open a file or directory.
  ///
  /// The returned file descriptor is not guaranteed to be the lowest-numbered
  /// file descriptor not currently open; it is randomized to prevent
  /// applications from depending on making assumptions about indexes, since
  /// this is error-prone in multi-threaded contexts. The returned file
  /// descriptor is guaranteed to be less than 2**31.
  ///
  /// Note: This is similar to `openat` in POSIX.
  ///
  /// @param[in] Path The relative path of the file or directory to open,
  /// relative to the `path_open::fd` directory.
  /// @param[in] OpenFlags The method by which to open the file.
  /// @param[in] FdFlags The method by which to open the file.
  /// @param[in] VFSFlags The method by which to open the file.
  /// @return The file descriptor of the file that has been opened, or WASI
  /// error.
  WasiExpect<INode> pathOpen(std::string Path, __wasi_oflags_t OpenFlags,
                             __wasi_fdflags_t FdFlags,
                             uint8_t VFSFlags) const noexcept;

  /// Read the contents of a symbolic link.
  ///
  /// Note: This is similar to `readlinkat` in POSIX.
  ///
  /// @param[in] Path The path of the symbolic link from which to read.
  /// @param[out] Buffer The buffer to which to write the contents of the
  /// symbolic link.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathReadlink(std::string Path,
                                Span<char> Buffer) const noexcept;

  /// Remove a directory.
  ///
  /// Return `errno::notempty` if the directory is not empty.
  ///
  /// Note: This is similar to `unlinkat(fd, path, AT_REMOVEDIR)` in POSIX.
  ///
  /// @param[in] Path The path to a directory to remove.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathRemoveDirectory(std::string Path) const noexcept;

  /// Rename a file or directory.
  ///
  /// Note: This is similar to `renameat` in POSIX.
  ///
  /// @param[in] Old The working directory at which the resolution of the old
  /// path starts.
  /// @param[in] OldPath The source path of the file or directory to rename.
  /// @param[in] New The working directory at which the resolution of the new
  /// path starts.
  /// @param[in] NewPath The destination path to which to rename the file or
  /// directory.
  /// @return Nothing or WASI error.
  static WasiExpect<void> pathRename(const INode &Old, std::string OldPath,
                                     const INode &New,
                                     std::string NewPath) noexcept;

  /// Create a symbolic link.
  ///
  /// Note: This is similar to `symlinkat` in POSIX.
  ///
  /// @param[in] OldPath The contents of the symbolic link.
  /// @param[in] NewPath The destination path at which to create the symbolic
  /// link.
  /// @return Nothing or WASI error
  WasiExpect<void> pathSymlink(std::string OldPath,
                               std::string NewPath) const noexcept;

  /// Unlink a file.
  ///
  /// Return `errno::isdir` if the path refers to a directory.
  ///
  /// Note: This is similar to `unlinkat(fd, path, 0)` in POSIX.
  ///
  /// @param[in] Path The path to a file to unlink.
  /// @return Nothing or WASI error.
  WasiExpect<void> pathUnlinkFile(std::string Path) const noexcept;

  /// Concurrently poll for the occurrence of a set of events.
  ///
  /// @param[in] NSubscriptions Both the number of subscriptions and events.
  /// @return Poll helper or WASI error
  static WasiExpect<Poller> pollOneoff(__wasi_size_t NSubscriptions) noexcept;

  /// Receive a message from a socket.
  ///
  /// Note: This is similar to `recv` in POSIX, though it also supports reading
  /// the data into multiple buffers in the manner of `readv`.
  ///
  /// @param[in] RiData List of scatter/gather vectors to which to store data.
  /// @param[in] RiFlags Message flags.
  /// @param[out] NRead Return the number of bytes stored in RiData.
  /// @param[out] RoFlags Return message flags.
  /// @return Nothing or WASI error.
  WasiExpect<void> sockRecv(Span<Span<uint8_t>> RiData,
                            __wasi_riflags_t RiFlags, __wasi_size_t &NRead,
                            __wasi_roflags_t &RoFlags) const noexcept;

  /// Send a message on a socket.
  ///
  /// Note: This is similar to `send` in POSIX, though it also supports writing
  /// the data from multiple buffers in the manner of `writev`.
  ///
  /// @param[in] SiData List of scatter/gather vectors to which to retrieve
  /// data.
  /// @param[in] SiFlags Message flags.
  /// @param[out] NWritten The number of bytes transmitted.
  /// @return Nothing or WASI error
  WasiExpect<void> sockSend(Span<Span<const uint8_t>> SiData,
                            __wasi_siflags_t SiFlags,
                            __wasi_size_t &NWritten) const noexcept;

  /// Shut down socket send and receive channels.
  ///
  /// Note: This is similar to `shutdown` in POSIX.
  ///
  /// @param[in] SdFlags Which channels on the socket to shut down.
  /// @return Nothing or WASI error
  WasiExpect<void> sockShutdown(__wasi_sdflags_t SdFlags) const noexcept;

  /// File type.
  WasiExpect<__wasi_filetype_t> filetype() const noexcept;

  /// Check if this inode is a directory.
  bool isDirectory() const noexcept;

  /// Check if this inode is a symbolic link.
  bool isSymlink() const noexcept;

  /// File size.
  WasiExpect<__wasi_filesize_t> filesize() const noexcept;

  /// Check if current user has execute permission on this inode.
  bool canBrowse() const noexcept;

private:
  friend class Poller;

  __wasi_filetype_t unsafeFiletype() const noexcept;

#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
public:
  using FdHolder::FdHolder;

private:
  mutable std::optional<struct stat> Stat;

  DirHolder Dir;

  WasiExpect<void> updateStat() const noexcept;
#endif
};

class Poller
#if WASMEDGE_OS_LINUX || WASMEDGE_OS_MACOS
    : public FdHolder
#endif
{
public:
  using CallbackType =
      std::function<void(__wasi_userdata_t, __wasi_errno_t, __wasi_eventtype_t,
                         __wasi_filesize_t, __wasi_eventrwflags_t)>;
  Poller(const Poller &) = delete;
  Poller &operator=(const Poller &) = delete;
  Poller(Poller &&RHS) noexcept = default;
  Poller &operator=(Poller &&RHS) noexcept = default;

  explicit Poller(__wasi_size_t Count);

  WasiExpect<void> clock(__wasi_clockid_t Clock, __wasi_timestamp_t Timeout,
                         __wasi_timestamp_t Precision,
                         __wasi_subclockflags_t Flags,
                         __wasi_userdata_t UserData) noexcept;

  WasiExpect<void> read(const INode &Fd, __wasi_userdata_t UserData) noexcept;

  WasiExpect<void> write(const INode &Fd, __wasi_userdata_t UserData) noexcept;

  WasiExpect<void> wait(CallbackType Callback) noexcept;

private:
  std::vector<__wasi_event_t> Events;

#if WASMEDGE_OS_LINUX
private:
  struct Timer : public FdHolder {
    Timer(const Timer &) = delete;
    Timer &operator=(const Timer &) = delete;
    Timer(Timer &&RHS) noexcept = default;
    Timer &operator=(Timer &&RHS) noexcept = default;
    constexpr Timer() noexcept = default;

    WasiExpect<void> create(__wasi_clockid_t Clock, __wasi_timestamp_t Timeout,
                            __wasi_timestamp_t Precision,
                            __wasi_subclockflags_t Flags) noexcept;

#if !__GLIBC_PREREQ(2, 8)
    FdHolder Notify;
    TimerHolder TimerId;
#endif
  };

  std::vector<Timer> Timers;
#endif
};

} // namespace WASI
} // namespace Host
} // namespace WasmEdge
