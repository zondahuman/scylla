/*
 * Copyright (C) 2017 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_MODULE core

#include "auth/resource.hh"

#include <sstream>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(root_of) {
    //
    // data
    //

    const auto dr = auth::resource::root_of(auth::resource_kind::data);
    BOOST_REQUIRE_EQUAL(dr.kind(), auth::resource_kind::data);

    const auto dv = auth::data_resource_view(dr);
    BOOST_REQUIRE(!dv.keyspace());
    BOOST_REQUIRE(!dv.table());

    //
    // role
    //

    const auto rr = auth::resource::root_of(auth::resource_kind::role);
    BOOST_REQUIRE_EQUAL(rr.kind(), auth::resource_kind::role);

    const auto rv = auth::role_resource_view(rr);
    BOOST_REQUIRE(!rv.role());
}

BOOST_AUTO_TEST_CASE(data) {
    const auto r1 = auth::resource::data("my_keyspace");
    BOOST_REQUIRE_EQUAL(r1.kind(), auth::resource_kind::data);
    const auto v1 = auth::data_resource_view(r1);

    BOOST_REQUIRE_EQUAL(*v1.keyspace(), "my_keyspace");
    BOOST_REQUIRE(!v1.table());

    const auto r2 = auth::resource::data("my_keyspace", "my_table");
    BOOST_REQUIRE_EQUAL(r2.kind(), auth::resource_kind::data);
    const auto v2 = auth::data_resource_view(r2);

    BOOST_REQUIRE_EQUAL(*v2.keyspace(), "my_keyspace");
    BOOST_REQUIRE_EQUAL(*v2.table(), "my_table");
}

BOOST_AUTO_TEST_CASE(role) {
    const auto r = auth::resource::role("joe");
    BOOST_REQUIRE_EQUAL(r.kind(), auth::resource_kind::role);

    const auto v = auth::role_resource_view(r);
    BOOST_REQUIRE_EQUAL(*v.role(), "joe");
}

BOOST_AUTO_TEST_CASE(from_name) {
    //
    // data
    //

    const auto dr1 = auth::resource::from_name("data");
    BOOST_REQUIRE_EQUAL(dr1, auth::resource::root_of(auth::resource_kind::data));

    const auto dr2 = auth::resource::from_name("data/my_keyspace");
    BOOST_REQUIRE_EQUAL(dr2, auth::resource::data("my_keyspace"));

    const auto dr3 = auth::resource::from_name("data/my_keyspace/my_table");
    BOOST_REQUIRE_EQUAL(dr3, auth::resource::data("my_keyspace", "my_table"));

    BOOST_REQUIRE_THROW(auth::resource::from_name("data/foo/bar/baz"), auth::invalid_resource_name);

    //
    // role
    //

    const auto rr1 = auth::resource::from_name("roles");
    BOOST_REQUIRE_EQUAL(rr1, auth::resource::root_of(auth::resource_kind::role));

    const auto rr2 = auth::resource::from_name("roles/joe");
    BOOST_REQUIRE_EQUAL(rr2, auth::resource::role("joe"));

    BOOST_REQUIRE_THROW(auth::resource::from_name("roles/joe/smith"), auth::invalid_resource_name);

    //
    // Generic errors.
    //

    BOOST_REQUIRE_THROW(auth::resource::from_name("animal/horse"), auth::invalid_resource_name);
    BOOST_REQUIRE_THROW(auth::resource::from_name(""), auth::invalid_resource_name);
}

BOOST_AUTO_TEST_CASE(name) {
    //
    // data
    //

    BOOST_REQUIRE_EQUAL(auth::resource::root_of(auth::resource_kind::data).name(), "data");
    BOOST_REQUIRE_EQUAL(auth::resource::data("my_keyspace").name(), "data/my_keyspace");
    BOOST_REQUIRE_EQUAL(auth::resource::data("my_keyspace", "my_table").name(), "data/my_keyspace/my_table");

    //
    // role
    //

    BOOST_REQUIRE_EQUAL(auth::resource::root_of(auth::resource_kind::role).name(), "roles");
    BOOST_REQUIRE_EQUAL(auth::resource::role("joe").name(), "roles/joe");
}

BOOST_AUTO_TEST_CASE(parent) {
    const auto r1 = auth::resource::data("my_keyspace", "my_table");

    const auto r2 = r1.parent();
    BOOST_REQUIRE(r2);
    BOOST_REQUIRE_EQUAL(*r2, auth::resource::data("my_keyspace"));

    const auto r3 = r2->parent();
    BOOST_REQUIRE(r3);
    BOOST_REQUIRE_EQUAL(*r3, auth::resource::root_of(auth::resource_kind::data));

    const auto r4 = r3->parent();
    BOOST_REQUIRE(!r4);
}

BOOST_AUTO_TEST_CASE(output) {
    //
    // data
    //

    BOOST_REQUIRE_EQUAL(sprint("%s", auth::resource::root_of(auth::resource_kind::data)), "<all keyspaces>");
    BOOST_REQUIRE_EQUAL(sprint("%s", auth::resource::data("my_keyspace")), "<keyspace my_keyspace>");
    BOOST_REQUIRE_EQUAL(sprint("%s", auth::resource::data("my_keyspace", "my_table")), "<table my_keyspace.my_table>");

    //
    // role
    //

    BOOST_REQUIRE_EQUAL(sprint("%s", auth::resource::root_of(auth::resource_kind::role)), "<all roles>");
    BOOST_REQUIRE_EQUAL(sprint("%s", auth::resource::role("joe")), "<role joe>");
}
