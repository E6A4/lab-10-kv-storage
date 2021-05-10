// Copyright 2020 Your Name <your_email>

#include "dbWrapper.hpp"

#include "iostream"

void rocksdbWrapper::getFamiliesFromBD() { //
  rocksdb::Options options; //опции для открытия бд
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly(options, path_, &db_); //открываю бд, если в бд есть семейства открываю методом ОпеФоРидОнли(аргументы опции,путь, ссылка на бд), возвращаю объект типа статус в котором будет инфа норм открылось или нет
  if (!status.ok()) std::cerr << status.ToString() << std::endl; // если статус не окей то вывожу статус
  db_->ListColumnFamilies(options, path_, &families_); //метод листколмнфамилес передаю ему опции и путь и ссылку на массив строк и он читает все названия семейств и кладет их в массив с названями семейств
  delete db_; //закрываю бд
}
void rocksdbWrapper::createDatabase() { //создаю пустую бд
  rocksdb::Options options;
  options.create_if_missing = true; //параметр крейтеиф миссинг - если база не существуе тто создать ее

  rocksdb::Status status = rocksdb::DB::Open(options, path_, &db_); //открываю бд
  if (!status.ok()) std::cerr << status.ToString() << std::endl;

  for (int i = 0; i < familyNum_; ++i) { //создаю 10 семейств
    rocksdb::ColumnFamilyHandle* cf; //дескриптор для семейств (дескриптор - указатель на объект с помощью которого можно проводить операции над семейством )
    status = db_->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), //создаю семейства, передаю опции для семейства,названия семейств фамилию+ номер счетчика (амперСФ это ссылка на дискриптор)
                                     "family_" + std::to_string(i), &cf);
    assert(status.ok());
    db_->DestroyColumnFamilyHandle(cf); //уничтожаю Хэндл(стрелка - указатель на метод)
  }

  delete db_; //удаляю бд из памяти
}

std::vector<std::string> rocksdbWrapper::getFamilyNum() { return families_ ;} //возвращаю массив семейств

void rocksdbWrapper::pushData() { //заполняю пустую бд данными
  rocksdb::Options options;

  std::vector<rocksdb::ColumnFamilyDescriptor> column_families; //оявляю вектор дескриптора семейств

  for (auto& family : families_) { //иду по каждому из массива семейств и добавляю его в массив дескриптора
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        family, rocksdb::ColumnFamilyOptions()));
  }

  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::Status status = rocksdb::DB::Open(rocksdb::DBOptions(), path_, //метод опен - если бд нет то он ее создаст, создаю ее и передаю опции,массив дескрипторов, хэндлс тоже дескрипторы
                                             column_families, &handles, &db_);

  assert(status.ok()); //проверяю что статус ок

  for (size_t i = 0; i < families_.size(); ++i) { //заполняю семейства данными
    for (int k = 0; k < columnSize_; ++k) { //
      status = db_->Put(rocksdb::WriteOptions(), handles[i], //метод пут  - положить значение в бд, в массиве хэндлс лежат все семейства, и я обращаю к каждому э-у массива хэндалс
                        rocksdb::Slice("key_" + std::to_string(k)), //создает фрагмент который ссылается на аргумент который я ему передаю, я передаю кей + номер счетчика
                        rocksdb::Slice("value_" + std::to_string(k))); //значение ключа
      assert(status.ok());
    }
  }
  for (auto handle : handles) { //каждый хэндлс вырубаю, применяю метод дестрой хэндлс
    status = db_->DestroyColumnFamilyHandle(handle);
    assert(status.ok());
  }

  delete db_;
}


void rocksdbWrapper::migrateDataToMap(std::string logLevel) { //метод открывает заполненную бд и переносит все данные в словарь
  rocksdb::Options options;

  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::Status status = rocksdb::DB::OpenForReadOnly( //открываю дефолтное семейство
      rocksdb::DBOptions(), path_, column_families, &handles, &db_);

  rocksdb::Iterator* it = db_->NewIterator(rocksdb::ReadOptions()); //итератор чтобы идти по всем значениям
  std::map<std::string, std::string> kvStorage;
  for (it->SeekToFirst(); it->Valid(); it->Next()) {
    kvStorage[it->key().ToString()] = it->value().ToString();
  }
  kvStorage.clear();
  assert(it->status().ok()); //77-83 читаю дефолтное семейство и заполняю словарь данными из этого семейства

  for (auto handle : handles) { //каждый хэндлс вырубаю, применяю метод дестрой хэндлс
    status = db_->DestroyColumnFamilyHandle(handle);
    assert(status.ok());
  }

  handles.clear(); //очищаю вектор дескрипторов (там хранится всего один дескр семейства дефолт)
  delete it; //очищаю итератор с помощью которого читаю все значения
  delete db_;

  for (const auto& family : families_) { //иду по всем остальным семействам
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        family, rocksdb::ColumnFamilyOptions()));

    status = rocksdb::DB::OpenForReadOnly(rocksdb::DBOptions(), path_,
                                          column_families, &handles, &db_);
    assert(status.ok());

    it = db_->NewIterator(rocksdb::ReadOptions());
    for (it->SeekToFirst(); it->Valid(); it->Next()) { //брал из доки как запустить итератор
      kvStorage[it->key().ToString()] = it->value().ToString(); //айти кей получаю ключ, айтивэлью получаю значение
    } //102-105 читаю данные из бд и переношу в словрь (ЗДЕСЬ Я ИМЕЮ ГОТОВЫЙ СЛОВРЬ СО ВСЕМИ ДАННЫМИ И ДАЛЕЕ ПЕРЕДАЮ ВСЕ ДАННЫЕ ХЭШЕРУ)
    hasherObj_.startHashing(family, kvStorage, logLevel); //запускаю хэшер

    kvStorage.clear(); //очищаю словарь семейств т.к. уже передал данные хэшеру
    assert(it->status().ok());

    for (auto handle : handles) {
      status = db_->DestroyColumnFamilyHandle(handle);
      assert(status.ok());
    }
  }
  delete it;
  delete db_;
}

void rocksdbWrapper::createOutputDatabase() { //метод создает базу в которую я буду закидывать уже хэшированные значения
  rocksdb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true; //если бд уже существует то ошибка
  rocksdb::Status status = rocksdb::DB::Open(options, path_, &db_); //создаю новую бд передаю аргументы
  if (!status.ok()) std::cerr << status.ToString() << std::endl;

  for (auto const& x : mapa_) {
    if (x.first ==  "default"){ //создаю семейство хэшированное
      continue; //переход на следующие итерацию
    }
    rocksdb::ColumnFamilyHandle* cf;
    status =
        db_->CreateColumnFamily(rocksdb::ColumnFamilyOptions(), x.first, &cf);
    assert(status.ok());
    db_->DestroyColumnFamilyHandle(cf);
  }

  delete db_;

  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;

  column_families.push_back(rocksdb::ColumnFamilyDescriptor(
      rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));

  for (auto& family : mapa_) {
    column_families.push_back(rocksdb::ColumnFamilyDescriptor(
        family.first, rocksdb::ColumnFamilyOptions()));
  }

  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  status = rocksdb::DB::Open(rocksdb::DBOptions(), path_, column_families,
                             &handles, &db_);

  assert(status.ok());
  for (size_t i = 0;i<mapa_.size();++i) {
    for (auto& kv : mapa_[handles[i]->GetName()]){
      status = db_->Put(rocksdb::WriteOptions(), handles[i],
                        rocksdb::Slice(kv.first),
                        rocksdb::Slice(kv.second));
      assert(status.ok());
    }
  }

  for (auto handle : handles) {
    status = db_->DestroyColumnFamilyHandle(handle);
    assert(status.ok());
  }
  delete db_;
}
