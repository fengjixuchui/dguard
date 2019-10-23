/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef dgiEncryptionTypes_TYPES_H
#define dgiEncryptionTypes_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/TBase.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <thrift/stdcxx.h>
#include "dgiCommonTypes_types.h"


namespace dgi {

struct HashAlgType {
  enum type {
    HAlg_Unknown = 0,
    HAlg_Crc32 = 1,
    HAlg_Md5 = 2,
    HAlg_Sha1 = 3,
    HAlg_Sha256 = 4,
    HAlg_Sha512 = 5,
    HAlg_Whirepool = 6
  };
};

extern const std::map<int, const char*> _HashAlgType_VALUES_TO_NAMES;

std::ostream& operator<<(std::ostream& out, const HashAlgType::type& val);

struct EncryptionAlgType {
  enum type {
    EAlg_Unknown = 0,
    EAlg_Grader = 1,
    EAlg_Aes = 2,
    EAlg_Aes256 = 3
  };
};

extern const std::map<int, const char*> _EncryptionAlgType_VALUES_TO_NAMES;

std::ostream& operator<<(std::ostream& out, const EncryptionAlgType::type& val);

class RequestEncryptFile;

class EncryptionFileInfo;

class ResponseFileInfo;

class RequestDecodeFile;

class ResponseDecodeFile;

typedef struct _RequestEncryptFile__isset {
  _RequestEncryptFile__isset() : filePath(false), useMasterPassword(false), encryptionKey(false), encodingAlgorithm(false) {}
  bool filePath :1;
  bool useMasterPassword :1;
  bool encryptionKey :1;
  bool encodingAlgorithm :1;
} _RequestEncryptFile__isset;

class RequestEncryptFile : public virtual ::apache::thrift::TBase {
 public:

  RequestEncryptFile(const RequestEncryptFile&);
  RequestEncryptFile& operator=(const RequestEncryptFile&);
  RequestEncryptFile() : filePath(), useMasterPassword(0), encryptionKey(), encodingAlgorithm((EncryptionAlgType::type)0) {
  }

  virtual ~RequestEncryptFile() throw();
  std::string filePath;
  bool useMasterPassword;
  std::string encryptionKey;
  EncryptionAlgType::type encodingAlgorithm;

  _RequestEncryptFile__isset __isset;

  void __set_filePath(const std::string& val);

  void __set_useMasterPassword(const bool val);

  void __set_encryptionKey(const std::string& val);

  void __set_encodingAlgorithm(const EncryptionAlgType::type val);

  bool operator == (const RequestEncryptFile & rhs) const
  {
    if (!(filePath == rhs.filePath))
      return false;
    if (!(useMasterPassword == rhs.useMasterPassword))
      return false;
    if (!(encryptionKey == rhs.encryptionKey))
      return false;
    if (!(encodingAlgorithm == rhs.encodingAlgorithm))
      return false;
    return true;
  }
  bool operator != (const RequestEncryptFile &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const RequestEncryptFile & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(RequestEncryptFile &a, RequestEncryptFile &b);

std::ostream& operator<<(std::ostream& out, const RequestEncryptFile& obj);

typedef struct _EncryptionFileInfo__isset {
  _EncryptionFileInfo__isset() : usedMasterPassword(false), encodingAlgorithm(false), originalFileSize(false), originalChecksum(false), keyChecksum(false) {}
  bool usedMasterPassword :1;
  bool encodingAlgorithm :1;
  bool originalFileSize :1;
  bool originalChecksum :1;
  bool keyChecksum :1;
} _EncryptionFileInfo__isset;

class EncryptionFileInfo : public virtual ::apache::thrift::TBase {
 public:

  EncryptionFileInfo(const EncryptionFileInfo&);
  EncryptionFileInfo& operator=(const EncryptionFileInfo&);
  EncryptionFileInfo() : usedMasterPassword(0), encodingAlgorithm((EncryptionAlgType::type)0), originalFileSize(0), originalChecksum(), keyChecksum() {
  }

  virtual ~EncryptionFileInfo() throw();
  bool usedMasterPassword;
  EncryptionAlgType::type encodingAlgorithm;
  int64_t originalFileSize;
  std::string originalChecksum;
  std::string keyChecksum;

  _EncryptionFileInfo__isset __isset;

  void __set_usedMasterPassword(const bool val);

  void __set_encodingAlgorithm(const EncryptionAlgType::type val);

  void __set_originalFileSize(const int64_t val);

  void __set_originalChecksum(const std::string& val);

  void __set_keyChecksum(const std::string& val);

  bool operator == (const EncryptionFileInfo & rhs) const
  {
    if (!(usedMasterPassword == rhs.usedMasterPassword))
      return false;
    if (!(encodingAlgorithm == rhs.encodingAlgorithm))
      return false;
    if (!(originalFileSize == rhs.originalFileSize))
      return false;
    if (!(originalChecksum == rhs.originalChecksum))
      return false;
    if (!(keyChecksum == rhs.keyChecksum))
      return false;
    return true;
  }
  bool operator != (const EncryptionFileInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const EncryptionFileInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(EncryptionFileInfo &a, EncryptionFileInfo &b);

std::ostream& operator<<(std::ostream& out, const EncryptionFileInfo& obj);

typedef struct _ResponseFileInfo__isset {
  _ResponseFileInfo__isset() : encryptedAlready(false), info(false), result(false) {}
  bool encryptedAlready :1;
  bool info :1;
  bool result :1;
} _ResponseFileInfo__isset;

class ResponseFileInfo : public virtual ::apache::thrift::TBase {
 public:

  ResponseFileInfo(const ResponseFileInfo&);
  ResponseFileInfo& operator=(const ResponseFileInfo&);
  ResponseFileInfo() : encryptedAlready(0) {
  }

  virtual ~ResponseFileInfo() throw();
  bool encryptedAlready;
  EncryptionFileInfo info;
   ::dgi::DgiResult result;

  _ResponseFileInfo__isset __isset;

  void __set_encryptedAlready(const bool val);

  void __set_info(const EncryptionFileInfo& val);

  void __set_result(const  ::dgi::DgiResult& val);

  bool operator == (const ResponseFileInfo & rhs) const
  {
    if (!(encryptedAlready == rhs.encryptedAlready))
      return false;
    if (!(info == rhs.info))
      return false;
    if (!(result == rhs.result))
      return false;
    return true;
  }
  bool operator != (const ResponseFileInfo &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ResponseFileInfo & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(ResponseFileInfo &a, ResponseFileInfo &b);

std::ostream& operator<<(std::ostream& out, const ResponseFileInfo& obj);

typedef struct _RequestDecodeFile__isset {
  _RequestDecodeFile__isset() : filePath(false), useMasterPassword(false), encryptionKey(false) {}
  bool filePath :1;
  bool useMasterPassword :1;
  bool encryptionKey :1;
} _RequestDecodeFile__isset;

class RequestDecodeFile : public virtual ::apache::thrift::TBase {
 public:

  RequestDecodeFile(const RequestDecodeFile&);
  RequestDecodeFile& operator=(const RequestDecodeFile&);
  RequestDecodeFile() : filePath(), useMasterPassword(0), encryptionKey() {
  }

  virtual ~RequestDecodeFile() throw();
  std::string filePath;
  bool useMasterPassword;
  std::string encryptionKey;

  _RequestDecodeFile__isset __isset;

  void __set_filePath(const std::string& val);

  void __set_useMasterPassword(const bool val);

  void __set_encryptionKey(const std::string& val);

  bool operator == (const RequestDecodeFile & rhs) const
  {
    if (!(filePath == rhs.filePath))
      return false;
    if (!(useMasterPassword == rhs.useMasterPassword))
      return false;
    if (!(encryptionKey == rhs.encryptionKey))
      return false;
    return true;
  }
  bool operator != (const RequestDecodeFile &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const RequestDecodeFile & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(RequestDecodeFile &a, RequestDecodeFile &b);

std::ostream& operator<<(std::ostream& out, const RequestDecodeFile& obj);

typedef struct _ResponseDecodeFile__isset {
  _ResponseDecodeFile__isset() : result(false), integrityCompromised(false) {}
  bool result :1;
  bool integrityCompromised :1;
} _ResponseDecodeFile__isset;

class ResponseDecodeFile : public virtual ::apache::thrift::TBase {
 public:

  ResponseDecodeFile(const ResponseDecodeFile&);
  ResponseDecodeFile& operator=(const ResponseDecodeFile&);
  ResponseDecodeFile() : integrityCompromised(0) {
  }

  virtual ~ResponseDecodeFile() throw();
   ::dgi::DgiResult result;
  bool integrityCompromised;

  _ResponseDecodeFile__isset __isset;

  void __set_result(const  ::dgi::DgiResult& val);

  void __set_integrityCompromised(const bool val);

  bool operator == (const ResponseDecodeFile & rhs) const
  {
    if (!(result == rhs.result))
      return false;
    if (!(integrityCompromised == rhs.integrityCompromised))
      return false;
    return true;
  }
  bool operator != (const ResponseDecodeFile &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ResponseDecodeFile & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

  virtual void printTo(std::ostream& out) const;
};

void swap(ResponseDecodeFile &a, ResponseDecodeFile &b);

std::ostream& operator<<(std::ostream& out, const ResponseDecodeFile& obj);

} // namespace

#endif
