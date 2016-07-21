/*
 * Copyright (C) 2015 ScyllaDB
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

#pragma once

#include "mutation.hh"

class mutation_assertion {
    mutation _m;
public:
    mutation_assertion(mutation m)
        : _m(std::move(m))
    { }

    mutation_assertion& is_equal_to(const mutation& other) {
        if (_m != other) {
            BOOST_FAIL(sprint("Mutations differ, expected %s\n ...but got: %s", other, _m));
        }
        if (other != _m) {
            BOOST_FAIL(sprint("Mutation inequality is not symmetric for %s\n ...and: %s", other, _m));
        }
        return *this;
    }

    mutation_assertion& is_not_equal_to(const mutation& other) {
        if (_m == other) {
            BOOST_FAIL(sprint("Mutations equal but expected to differ: %s\n ...and: %s", other, _m));
        }
        return *this;
    }

    mutation_assertion& has_schema(schema_ptr s) {
        if (_m.schema() != s) {
            BOOST_FAIL(sprint("Expected mutation of schema %s, but got %s", *s, *_m.schema()));
        }
        return *this;
    }

    // Verifies that mutation data remains unchanged when upgraded to the new schema
    void is_upgrade_equivalent(schema_ptr new_schema) {
        mutation m2 = _m;
        m2.upgrade(new_schema);
        BOOST_REQUIRE(m2.schema() == new_schema);
        mutation_assertion(m2).is_equal_to(_m);

        mutation m3 = m2;
        m3.upgrade(_m.schema());
        BOOST_REQUIRE(m3.schema() == _m.schema());
        mutation_assertion(m3).is_equal_to(_m);
        mutation_assertion(m3).is_equal_to(m2);
    }
};

static inline
mutation_assertion assert_that(mutation m) {
    return { std::move(m) };
}

static inline
mutation_assertion assert_that(streamed_mutation sm) {
    auto mo = mutation_from_streamed_mutation(std::move(sm)).get0();
    return { std::move(*mo) };
}

class mutation_opt_assertions {
    mutation_opt _mo;
public:
    mutation_opt_assertions(mutation_opt mo) : _mo(std::move(mo)) {}

    mutation_assertion has_mutation() {
        if (!_mo) {
            BOOST_FAIL("Expected engaged mutation_opt, but found not");
        }
        return { *_mo };
    }

    void has_no_mutation() {
        if (_mo) {
            BOOST_FAIL("Expected disengaged mutation_opt");
        }
    }
};

static inline
mutation_opt_assertions assert_that(mutation_opt mo) {
    return { std::move(mo) };
}

static inline
mutation_opt_assertions assert_that(streamed_mutation_opt smo) {
    auto mo = mutation_from_streamed_mutation(std::move(smo)).get0();
    return { std::move(mo) };
}

class streamed_mutation_assertions {
    streamed_mutation _sm;
    clustering_key::equality _ck_eq;
public:
    streamed_mutation_assertions(streamed_mutation sm)
        : _sm(std::move(sm)), _ck_eq(*_sm.schema()) { }

    streamed_mutation_assertions& produces_static_row() {
        auto mfopt = _sm().get0();
        if (!mfopt) {
            BOOST_FAIL("Expected static row, got end of stream");
        }
        if (mfopt->mutation_fragment_kind() != mutation_fragment::kind::static_row) {
            BOOST_FAIL(sprint("Expected static row, got: %s", mfopt->mutation_fragment_kind()));
        }
        return *this;
    }

    streamed_mutation_assertions& produces(mutation_fragment::kind k, std::vector<int> ck_elements) {
        std::vector<bytes> ck_bytes;
        for (auto&& e : ck_elements) {
            ck_bytes.emplace_back(int32_type->decompose(e));
        }
        auto ck = clustering_key_prefix::from_exploded(*_sm.schema(), std::move(ck_bytes));

        auto mfopt = _sm().get0();
        if (!mfopt) {
            BOOST_FAIL(sprint("Expected mutation fragment %s, got end of stream", ck));
        }
        if (mfopt->mutation_fragment_kind() != k) {
            BOOST_FAIL(sprint("Expected mutation fragment kind %s, got: %s", k, mfopt->mutation_fragment_kind()));
        }
        if (!_ck_eq(mfopt->key(), ck)) {
            BOOST_FAIL(sprint("Expected key %s, got: %s", ck, mfopt->key()));
        }
        return *this;
    }

    streamed_mutation_assertions& produces_end_of_stream() {
        auto mfopt = _sm().get0();
        BOOST_REQUIRE(!mfopt);
        if (mfopt) {
            BOOST_FAIL(sprint("Expected end of stream, got: %s", mfopt->mutation_fragment_kind()));
        }
        return *this;
    }
};

static inline streamed_mutation_assertions assert_that_stream(streamed_mutation sm)
{
    return streamed_mutation_assertions(std::move(sm));
}