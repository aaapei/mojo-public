// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/service_provider_impl.h"

#include <utility>

#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/public/interfaces/bindings/tests/ping_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class ServiceProviderImplTest : public testing::Test {
 public:
  ServiceProviderImplTest() {}
  ~ServiceProviderImplTest() override { loop_.RunUntilIdle(); }

  RunLoop& loop() { return loop_; }

 protected:
  void QuitLoop(bool ok) {
    EXPECT_TRUE(ok);
    loop_.Quit();
  }

 private:
  Environment env_;
  RunLoop loop_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ServiceProviderImplTest);
};

class PingServiceImpl : public test::PingService {
 public:
  PingServiceImpl(InterfaceRequest<test::PingService> ping_service_request)
      : strong_binding_(this, std::move(ping_service_request)) {}
  ~PingServiceImpl() override {}

  // |test::PingService|:
  void Ping(const PingCallback& callback) override { callback.Run(); }

 private:
  StrongBinding<test::PingService> strong_binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(PingServiceImpl);
};

TEST_F(ServiceProviderImplTest, Basic) {
  const char kPing1[] = "Ping1";
  const char kPing2[] = "Ping2";
  const char kPing3[] = "Ping3";

  ServiceProviderPtr sp;
  ServiceProviderImpl impl(GetProxy(&sp));

  impl.AddServiceNew<test::PingService>(
      [](const ConnectionContext& connection_context,
         InterfaceRequest<test::PingService> ping_service_request) {
        new PingServiceImpl(std::move(ping_service_request));
      },
      kPing1);

  impl.AddServiceNew<test::PingService>(
      [](const ConnectionContext& connection_context,
         InterfaceRequest<test::PingService> ping_service_request) {
        new PingServiceImpl(std::move(ping_service_request));
      },
      kPing2);

  {
    test::PingServicePtr ping1;
    ConnectToService(sp.get(), GetProxy(&ping1), kPing1);
    ping1.set_connection_error_handler([this] { QuitLoop(false); });
    ping1->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  {
    test::PingServicePtr ping2;
    ConnectToService(sp.get(), GetProxy(&ping2), kPing2);
    ping2.set_connection_error_handler([this] { QuitLoop(false); });
    ping2->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // "Ping3" isn't actually registered!
  {
    test::PingServicePtr ping3;
    ConnectToService(sp.get(), GetProxy(&ping3), kPing3);
    ping3.set_connection_error_handler([this] { QuitLoop(true); });
    ping3->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  sp.reset();
  loop().RunUntilIdle();
}

TEST_F(ServiceProviderImplTest, CloseAndRebind) {
  const char kPing[] = "Ping";

  ServiceProviderPtr sp1;
  ServiceProviderImpl impl(GetProxy(&sp1));

  impl.AddServiceNew<test::PingService>(
      [](const ConnectionContext& connection_context,
         InterfaceRequest<test::PingService> ping_service_request) {
        new PingServiceImpl(std::move(ping_service_request));
      },
      kPing);

  {
    test::PingServicePtr ping;
    ConnectToService(sp1.get(), GetProxy(&ping), kPing);
    ping.set_connection_error_handler([this] { QuitLoop(false); });
    ping->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  impl.Close();
  sp1.reset();
  loop().RunUntilIdle();

  ServiceProviderPtr sp2;
  impl.Bind(GetProxy(&sp2));

  {
    test::PingServicePtr ping;
    ConnectToService(sp2.get(), GetProxy(&ping), kPing);
    ping.set_connection_error_handler([this] { QuitLoop(false); });
    ping->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // Can close multiple times.
  impl.Close();
  impl.Close();
  sp2.reset();
  loop().RunUntilIdle();
}

TEST_F(ServiceProviderImplTest, Bind) {
  const char kPing[] = "Ping";

  ServiceProviderPtr sp;
  ServiceProviderImpl impl;

  impl.Bind(GetProxy(&sp));

  impl.AddServiceNew<test::PingService>(
      [](const ConnectionContext& connection_context,
         InterfaceRequest<test::PingService> request) {
        new PingServiceImpl(std::move(request));
      },
      kPing);

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kPing);
    ping.set_connection_error_handler([this] { QuitLoop(false); });
    ping->Ping([this] { QuitLoop(true); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  sp.reset();
  loop().RunUntilIdle();
}

class FauxServiceProvider : public ServiceProvider {
 public:
  explicit FauxServiceProvider(
      std::function<void(const std::string& service_name)>
          on_connect_to_service)
      : on_connect_to_service_(on_connect_to_service) {}
  ~FauxServiceProvider() override {}

  // |ServiceProvider|:
  void ConnectToService(const String& service_name,
                        ScopedMessagePipeHandle client_handle) override {
    on_connect_to_service_(service_name.get());
  }

 private:
  std::function<void(const std::string& service_name)> on_connect_to_service_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(FauxServiceProvider);
};

TEST_F(ServiceProviderImplTest, FallbackServiceProvider) {
  const char kWhatever[] = "Whatever";

  ServiceProviderPtr sp;
  ServiceProviderImpl impl(GetProxy(&sp));

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kWhatever);
    ping.set_connection_error_handler([this] { QuitLoop(true); });
    ping->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  bool was_run = false;
  FauxServiceProvider fallback_sp(
      [this, &kWhatever, &was_run](const std::string& service_name) {
        EXPECT_EQ(kWhatever, service_name);
        was_run = true;
      });
  impl.set_fallback_service_provider(&fallback_sp);

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kWhatever);
    ping.set_connection_error_handler([this] { QuitLoop(true); });
    EXPECT_FALSE(was_run);
    ping->Ping([this] { QuitLoop(false); });
    loop().Run();
    EXPECT_TRUE(was_run);
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  // Clear the fallback.
  impl.set_fallback_service_provider(nullptr);
  was_run = false;

  {
    test::PingServicePtr ping;
    ConnectToService(sp.get(), GetProxy(&ping), kWhatever);
    ping.set_connection_error_handler([this] { QuitLoop(true); });
    ping->Ping([this] { QuitLoop(false); });
    loop().Run();
  }
  loop().RunUntilIdle();  // Run stuff caused by destructors.

  sp.reset();
  loop().RunUntilIdle();

  EXPECT_FALSE(was_run);
}

}  // namespace
}  // namespace mojo
