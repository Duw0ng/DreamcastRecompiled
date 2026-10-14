package main

// buildRunGameArgs mirrors the options supported by run_game.bat. The full
// launch path is preferred over the old portable-only bootstrap launcher,
// because it preserves per-title caches and ABI1 incremental native patches.
type LaunchOptions struct {
	Renderer, AudioMode, BuildMode, FPU                                          int
	Perf, Debug, PVRProfile, MapleTrace, GDROMTrace                              bool
	Heartbeats, ShowConsole, NativeSeeds, NativePatches, Interpreter             bool
	InterpTrace, CodeRelocTrace, SPGFastForward, HotRecovery                     bool
	AutoPortableSync, RebuildRuntime, RebuildNetwork, RebuildMaple, RebuildGDROM bool
	HotPC                                                                        bool
}

func buildRunGameArgs(o LaunchOptions) []string {
	args := []string{"--no-pause"}
	switch o.Renderer {
	case 1:
		args = append(args, "--renderer=dx11")
	case 2:
		args = append(args, "--renderer=software")
	default:
		args = append(args, "--renderer=auto")
	}
	if o.AudioMode == 1 {
		args = append(args, "--audio-basic")
	}
	if o.AudioMode == 2 {
		args = append(args, "--no-audio")
	}
	if o.Perf {
		args = append(args, "--perf")
	}
	if o.Debug {
		args = append(args, "--debug")
	}
	if o.PVRProfile {
		args = append(args, "--profile")
	}
	if o.MapleTrace {
		args = append(args, "--maple-trace")
	}
	if o.GDROMTrace {
		args = append(args, "--gdrom-trace")
	}
	if o.Heartbeats {
		args = append(args, "--heartbeat-on")
	} else {
		args = append(args, "--heartbeat-off")
	}
	if o.BuildMode == 1 {
		args = append(args, "--reuse")
	}
	if o.BuildMode == 2 {
		args = append(args, "--recompile-game")
	}
	if !o.NativeSeeds {
		args = append(args, "--no-native-seeds")
	}
	if !o.NativePatches {
		args = append(args, "--no-native-patches")
	}
	if !o.Interpreter {
		args = append(args, "--no-interp")
	}
	if o.InterpTrace {
		args = append(args, "--interp-trace")
	}
	if o.CodeRelocTrace {
		args = append(args, "--code-reloc-trace")
	}
	if !o.SPGFastForward {
		args = append(args, "--spg-ff-off")
	}
	if !o.HotRecovery {
		args = append(args, "--hot-recovery-off")
	}
	if o.HotPC {
		args = append(args, "--hotpc")
	}
	if o.FPU == 1 {
		args = append(args, "--fpu-regionplus")
	} else {
		args = append(args, "--fpu-superblock")
	}
	if !o.AutoPortableSync {
		args = append(args, "--no-portable-sync")
	}
	if o.RebuildRuntime {
		args = append(args, "--rebuild-runtime")
	}
	if o.RebuildNetwork {
		args = append(args, "--rebuild-network")
	}
	if o.RebuildMaple {
		args = append(args, "--rebuild-maple")
	}
	if o.RebuildGDROM {
		args = append(args, "--rebuild-gdrom")
	}
	return args
}
