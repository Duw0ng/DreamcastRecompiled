package main

import (
	"strings"
	"testing"
)

func TestIncrementalDefaults(t *testing.T) {
	x := buildRunGameArgs(LaunchOptions{NativeSeeds: true, NativePatches: true, Interpreter: true, SPGFastForward: true, HotRecovery: true})
	s := strings.Join(x, " ")
	for _, required := range []string{"--heartbeat-off", "--no-portable-sync", "--renderer=auto"} {
		if !strings.Contains(s, required) {
			t.Fatalf("missing %q", required)
		}
	}
	for _, forbidden := range []string{"--recompile-game", "--reuse", "--no-native-seeds", "--no-native-patches", "--no-interp"} {
		if strings.Contains(s, forbidden) {
			t.Fatalf("unexpected %q", forbidden)
		}
	}
}
func TestRepairAndDebug(t *testing.T) {
	x := buildRunGameArgs(LaunchOptions{Renderer: 2, AudioMode: 2, BuildMode: 1, InterpTrace: true, NativeSeeds: true, NativePatches: true, Interpreter: true, RebuildNetwork: true})
	s := strings.Join(x, " ")
	for _, required := range []string{"--renderer=software", "--no-audio", "--reuse", "--interp-trace", "--rebuild-network"} {
		if !strings.Contains(s, required) {
			t.Fatalf("missing %q", required)
		}
	}
}
