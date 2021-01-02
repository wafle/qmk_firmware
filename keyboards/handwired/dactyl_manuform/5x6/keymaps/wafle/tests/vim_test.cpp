#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "vim.h"
#include <vector>

using ::testing::ContainerEq;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::IsEmpty;
using ::testing::Test;
using ::testing::Values;
using ::testing::WithParamInterface;

void write(std::vector<keypress> keycode_seq) {
    for (auto keycode : keycode_seq) {
        process_record_user(keycode.first, keycode.second);
    }
}

keypress P(uint16_t keycode) { return {keycode, &pressed}; }
keypress R(uint16_t keycode) { return {keycode, &depressed}; }
#define T(keycode) P(keycode), R(keycode)
#define LINE_START T(KC_END), T(KC_HOME), T(KC_HOME)

class VimTest : public Test {
    void SetUp() {
        insert_mode();
        keycodes.clear();
        normal_mode();
    }
};

class NavigationTest
    : public VimTest,
      public testing::WithParamInterface<std::pair<uint16_t, uint16_t>> {};

TEST_P(NavigationTest, Navigate) {
    auto x = GetParam();
    if (LSFT(x.first) == x.first) {
        write({P(KC_LSHIFT)});
        x.first &= ~QK_LSFT;
    }
    write({P(x.first)});
    EXPECT_THAT(keycodes, ElementsAre(P(x.second))) << "press";
    keycodes.clear();
    write({R(x.first)});
    EXPECT_THAT(keycodes, ElementsAre(R(x.second))) << "release";
}

INSTANTIATE_TEST_CASE_P(
    AllNavigation, NavigationTest,
    Values(std::pair<uint16_t, uint16_t>{KC_H, KC_LEFT},
           std::pair<uint16_t, uint16_t>{KC_J, KC_DOWN},
           std::pair<uint16_t, uint16_t>{KC_K, KC_UP},
           std::pair<uint16_t, uint16_t>{KC_L, KC_RIGHT},
           std::pair<uint16_t, uint16_t>{KC_W, LCTL(KC_RIGHT)},
           std::pair<uint16_t, uint16_t>{KC_B, LCTL(KC_LEFT)},
           std::pair<uint16_t, uint16_t>{LSFT(KC_4), KC_END},
           std::pair<uint16_t, uint16_t>{LSFT(KC_6), KC_HOME}));

TEST_F(VimTest, 2j) {
    write({T(KC_2), P(KC_J)});
    EXPECT_THAT(keycodes, ElementsAre(P(KC_DOWN), R(KC_DOWN), P(KC_DOWN)));
    keycodes.clear();
    write({R(KC_J)});
    EXPECT_THAT(keycodes, ElementsAre(R(KC_DOWN)));
}

TEST_F(VimTest, d2w) {
    write({T(KC_D), T(KC_2), P(KC_W)});
    auto crP = P(LCTL(KC_RIGHT));
    auto crR = R(LCTL(KC_RIGHT));
    EXPECT_THAT(keycodes, ElementsAre(P(KC_LSHIFT), crP, crR, crP, crR,
                                      R(KC_LSHIFT), T(LCTL(KC_X))));
}

TEST_F(VimTest, Y1p) {
    write({P(KC_LSHIFT), T(KC_Y), R(KC_LSHIFT)});
    EXPECT_THAT(keycodes,
                ElementsAreArray({LINE_START, P(KC_LSHIFT), T(KC_END),
                                  R(KC_LSHIFT), T(LCTL(KC_C)), T(KC_LEFT)}));
    keycodes.clear();
    write({T(KC_1), T(KC_P)});
    EXPECT_THAT(keycodes,
                ElementsAre(T(KC_END), T(KC_ENTER), T(LCTL(KC_V)), T(KC_HOME)));
}

TEST_F(VimTest, D) {
    write({P(KC_LSHIFT), T(KC_D)});
    EXPECT_THAT(keycodes, ElementsAre(P(KC_LSHIFT), T(KC_END), R(KC_LSHIFT),
                                      T(LCTL(KC_X))));
}

TEST_F(VimTest, c3k) {
    write({T(KC_C), T(KC_3), P(KC_K)});
    EXPECT_THAT(keycodes, ElementsAreArray({T(KC_END), P(KC_LSHIFT), T(KC_UP),
                                            T(KC_UP), T(KC_HOME), R(KC_LSHIFT),
                                            T(LCTL(KC_X)), T(KC_DEL)}));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, indent) {
    write({P(KC_LSHIFT), T(KC_DOT), T(KC_DOT), R(KC_LSHIFT)});
    EXPECT_THAT(keycodes, ElementsAre(LINE_START, T(KC_SPACE), T(KC_SPACE)));
}

TEST_F(VimTest, indent1kxdeindent) {
    write({P(KC_LSHIFT), T(KC_DOT), R(KC_LSHIFT), T(KC_1), P(KC_K)});
    auto indent_matcher = ElementsAreArray(
        {LINE_START, T(KC_SPACE), T(KC_SPACE), T(KC_UP), T(KC_HOME),
         T(KC_SPACE), T(KC_SPACE), T(KC_DOWN), T(KC_HOME)});
    EXPECT_THAT(keycodes, indent_matcher);
    keycodes.clear();

    write({T(KC_DOT)});
    EXPECT_THAT(keycodes, indent_matcher);

    keycodes.clear();
    write({P(KC_LSHIFT), T(KC_COMM), R(KC_LSHIFT), T(KC_1), P(KC_K)});
    EXPECT_THAT(keycodes, ElementsAreArray({LINE_START, T(KC_DEL), T(KC_DEL),
                                            T(KC_UP), T(KC_DEL), T(KC_DEL),
                                            T(KC_DOWN), T(KC_HOME)}));
}

TEST_F(VimTest, 2J) {
    write({T(KC_2), P(KC_LSHIFT), P(KC_J)});
    EXPECT_THAT(keycodes,
                ElementsAreArray({T(KC_END), T(KC_DEL), T(KC_SPACE), T(KC_END),
                                  T(KC_DEL), T(KC_SPACE)}));
}

TEST_F(VimTest, i) {
    write({T(KC_I)});
    EXPECT_THAT(keycodes, ElementsAre());
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, I) {
    write({P(KC_LSHIFT), T(KC_I)});
    EXPECT_THAT(keycodes, ElementsAre(T(KC_HOME)));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, a) {
    write({T(KC_A)});
    EXPECT_THAT(keycodes, ElementsAre(T(KC_RIGHT)));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, A) {
    write({P(KC_LSHIFT), T(KC_A)});
    EXPECT_THAT(keycodes, ElementsAre(T(KC_END)));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, o) {
    write({T(KC_O)});
    EXPECT_THAT(keycodes, ElementsAre(T(KC_END), T(KC_ENTER)));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, O) {
    write({P(KC_LSHIFT), T(KC_O)});
    EXPECT_THAT(keycodes, ElementsAre(T(KC_HOME), T(KC_ENTER), T(KC_UP)));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, 3s) {
    write({T(KC_3), T(KC_S)});
    EXPECT_THAT(keycodes, ElementsAre(T(KC_DEL), T(KC_DEL), T(KC_DEL)));
    EXPECT_FALSE(in_normal_mode);
}

TEST_F(VimTest, 34u5dot) {
    write({T(KC_3), T(KC_4), T(KC_U), T(KC_5), T(KC_DOT)});
    std::vector<keypress> expected;
    for (int i = 0; i < 34 + 34 * 5; ++i) {
        expected.push_back(P(LCTL(KC_Z)));
        expected.push_back(R(LCTL(KC_Z)));
    }
    EXPECT_THAT(keycodes, ElementsAreArray(expected));
}
