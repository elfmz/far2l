//------------------------------------------------------------------------------
// testnetbox_04.cpp
// testnetbox_04 --run_test=netbox/test1 --log_level=all 2>&1 | tee res.txt
//------------------------------------------------------------------------------

#include <Classes.hpp>
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "boostdefines.hpp"
#define BOOST_TEST_MODULE "testnetbox_04"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <Common.h>
#include <FileBuffer.h>

#include "TestTexts.h"
#include "FarPlugin.h"
#include "testutils.h"

#include "ne_session.h"
#include "ne_request.h"

// #include "dl/include.hpp"

#include "calculator.hpp"
#include "DynamicQueue.hpp"

using namespace boost::unit_test;

/*******************************************************************************
            test suite
*******************************************************************************/

class base_fixture_t
{
public:
  base_fixture_t()
  {
    InitPlatformId();
  }
  virtual ~base_fixture_t()
  {
  }

public:
protected:
  static int read_block(void * udata, const char * data, size_t len)
  {
    std::vector<unsigned char> *vec = static_cast<std::vector<unsigned char> *>(udata);
    for (unsigned int n = 0; n < len; ++n)
      vec->push_back((unsigned char)data[n]);
  }
};

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(netbox)

BOOST_FIXTURE_TEST_CASE(test1, base_fixture_t)
{
#if 0
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

  ne_session * sess = ne_session_create("http", "farmanager.com", 80);
  ne_request * req = ne_request_create(sess, "GET", "/svn/trunk/unicode_far/vbuild.m4");
  std::vector<unsigned char> vec;
  // ne_add_response_body_reader(req, ne_accept_2xx, read_block, &vec);
  int rv = ne_request_dispatch(req);
  BOOST_TEST_MESSAGE("rv = " << rv);
  if (rv)
  {
    BOOST_FAIL("Request failed: " << ne_get_error(sess));
  }
  BOOST_CHECK(rv == 0);
  const ne_status * statstruct = ne_get_status(req);
  BOOST_TEST_MESSAGE("statstruct->code = " << statstruct->code);
  BOOST_TEST_MESSAGE("statstruct->reason_phrase = " << statstruct->reason_phrase);
  if (vec.size() > 0)
  {
    BOOST_TEST_MESSAGE("response = " << (char *)&vec[0]);
  }
  ne_request_destroy(req);
  ne_session_destroy(sess);
  
  WSACleanup();
#endif
}

/*BOOST_FIXTURE_TEST_CASE(test2, base_fixture_t)
{
  team::calculator calc("calculator_dll.dll");
  BOOST_TEST_MESSAGE("sum = " << calc.sum(10, 20));
  BOOST_TEST_MESSAGE("mul = " << calc.mul(10, 20));
  BOOST_TEST_MESSAGE("sqrt = " << calc.sqrt(25));
}*/

BOOST_FIXTURE_TEST_CASE(test3, base_fixture_t)
{
  DynamicQueue<int> q;
  q.Reserve(10);
  q.Put(1);
  q.Put(2);
  q.Put(3);
  int val = q.Get();
  BOOST_TEST_MESSAGE("val = " << val);
  BOOST_CHECK(val == 1);
  val = q.Get();
  BOOST_TEST_MESSAGE("val = " << val);
  BOOST_CHECK(val == 2);
  val = q.Get();
  BOOST_TEST_MESSAGE("val = " << val);
  BOOST_CHECK(val == 3);
}

BOOST_AUTO_TEST_SUITE_END()
