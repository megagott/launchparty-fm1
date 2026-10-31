import unittest,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from import_shows import slice_scene,encode,decode

class ImportTests(unittest.TestCase):
    def test_cut_carries_lit_pad_and_its_release(self):
        e=[(0,1,36,53),(1920,0,36,0),(2000,1,123,49),(2500,0,123,0)]
        s=slice_scene(e,960,2880)
        self.assertEqual(s['initial'][36],53)
        self.assertEqual(s['events'],[[960,36,0],[1040,123,49],[1540,123,0]])
        self.assertEqual(decode(encode(s)),s)
    def test_boundary_events_are_not_duplicated(self):
        e=[(0,1,36,53),(960,0,36,0),(960,1,36,49),(1920,0,36,0)]
        a=slice_scene(e,0,960);b=slice_scene(e,960,1920)
        self.assertEqual(a['events'],[[0,36,53]])
        self.assertEqual(b['initial'][36],53)
        self.assertEqual(b['events'],[[0,36,0],[0,36,49]])
    def test_corrupt_stream_refused(self):
        s=slice_scene([(0,1,36,53)],0,960);b=encode(s)
        with self.assertRaises(ValueError):decode(b[:-1])
        with self.assertRaises(ValueError):decode(b+b'junk')

if __name__=='__main__':unittest.main()
