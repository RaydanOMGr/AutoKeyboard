package me.andreasmelone.autokeyboard.mixin;

import me.andreasmelone.autokeyboard.AutokeyboardClient;
import me.andreasmelone.autokeyboard.pojav.PojavIntegrateAndroidNative;
import net.minecraft.client.gui.components.EditBox;
import org.spongepowered.asm.mixin.Mixin;
import org.spongepowered.asm.mixin.injection.At;
import org.spongepowered.asm.mixin.injection.Inject;
import org.spongepowered.asm.mixin.injection.callback.CallbackInfo;

@Mixin(EditBox.class)
public abstract class EditBoxMixin {
    @Inject(
            method = "setFocused",
            at = @At(value = "INVOKE", target = "Lnet/minecraft/client/gui/components/AbstractWidget;setFocused(Z)V")
    )
    public void onFocused(boolean bl, CallbackInfo ci) {
        if(!AutokeyboardClient.isInitializedNative()) return;
        PojavIntegrateAndroidNative.setKeyboardState(bl);
    }
}
