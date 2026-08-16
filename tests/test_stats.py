import math
import unittest

from scripts.stats import (
    confidence_interval_95,
    sample_mean,
    sample_standard_deviation,
)


class StatsTests(unittest.TestCase):
    def test_sample_mean_calculates_arithmetic_mean(self):
        self.assertEqual(sample_mean([2, 4, 6, 8]), 5.0)

    def test_sample_mean_accepts_iterators(self):
        self.assertEqual(sample_mean(value for value in [1, 2, 3]), 2.0)

    def test_sample_mean_rejects_empty_sample(self):
        with self.assertRaises(ValueError):
            sample_mean([])

    def test_sample_standard_deviation_uses_n_minus_one(self):
        values = [2, 4, 4, 4, 5, 5, 7, 9]
        expected = math.sqrt(32 / 7)

        self.assertAlmostEqual(sample_standard_deviation(values), expected)

    def test_sample_standard_deviation_is_zero_for_constant_sample(self):
        self.assertEqual(sample_standard_deviation([3, 3, 3]), 0.0)

    def test_sample_standard_deviation_rejects_single_value(self):
        with self.assertRaises(ValueError):
            sample_standard_deviation([10])

    def test_confidence_interval_95_uses_project_formula(self):
        values = [10, 12, 14, 16, 18]
        mean = 14.0
        sample_standard_deviation = math.sqrt(10)
        margin = 1.96 * sample_standard_deviation / math.sqrt(len(values))

        lower, upper = confidence_interval_95(values)

        self.assertAlmostEqual(lower, mean - margin)
        self.assertAlmostEqual(upper, mean + margin)

    def test_confidence_interval_is_degenerate_for_constant_sample(self):
        self.assertEqual(confidence_interval_95([7, 7, 7]), (7.0, 7.0))

    def test_confidence_interval_rejects_single_value(self):
        with self.assertRaises(ValueError):
            confidence_interval_95([10])

    def test_functions_reject_non_finite_values(self):
        for invalid_value in (math.nan, math.inf, -math.inf):
            with self.subTest(invalid_value=invalid_value):
                with self.assertRaises(ValueError):
                    confidence_interval_95([1, invalid_value])

    def test_functions_reject_none(self):
        with self.assertRaises(TypeError):
            sample_mean(None)


if __name__ == "__main__":
    unittest.main()
