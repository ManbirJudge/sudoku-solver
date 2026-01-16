package com.example.sudokusolver

import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.net.toUri
import androidx.core.view.GravityCompat
import androidx.core.view.WindowCompat
import androidx.navigation.NavController
import androidx.navigation.fragment.NavHostFragment
import androidx.navigation.ui.AppBarConfiguration
import androidx.navigation.ui.navigateUp
import androidx.navigation.ui.setupActionBarWithNavController
import androidx.navigation.ui.setupWithNavController
import androidx.preference.PreferenceManager
import com.example.sudokusolver.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    private lateinit var navController: NavController
    private lateinit var appBarConfig: AppBarConfiguration

    private lateinit var settings: SharedPreferences

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        settings = PreferenceManager.getDefaultSharedPreferences(this)
        val settingsTheme = settings.getString("theme", "sys")

        AppCompatDelegate.setDefaultNightMode(when (settingsTheme)
        {
            "sys" ->   AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
            "light" -> AppCompatDelegate.MODE_NIGHT_NO
            "dark" ->  AppCompatDelegate.MODE_NIGHT_YES
            else ->    AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
        })

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        WindowCompat.setDecorFitsSystemWindows(window, true)

        setSupportActionBar(binding.mainActionBar)

        navController = (supportFragmentManager.findFragmentById(R.id.main_nav_host) as NavHostFragment).navController

        appBarConfig = AppBarConfiguration(
            setOf(R.id.nav_camera, R.id.nav_manual),
            binding.mainDrawerLayout
        )

        setupActionBarWithNavController(navController, appBarConfig)
        binding.mainBottomNav.setupWithNavController(navController)
        binding.mainDrawer.setupWithNavController(navController)

        binding.mainDrawer.menu.findItem(R.id.nav_donation).setOnMenuItemClickListener {
            binding.mainDrawerLayout.closeDrawer(GravityCompat.START)
            startActivity(Intent(
                Intent.ACTION_VIEW,
                "https://manbir-judge.netlify.app/buy-me-a-coffee".toUri()
            ))
            true
        }

        navController.addOnDestinationChangedListener { _, destination, _ ->
            when (destination.id) {
                R.id.nav_camera, R.id.nav_manual -> {
                    binding.mainBottomNav.visibility = View.VISIBLE
                    binding.mainActionBar.title = getString(R.string.app_name)
                }
                else -> {
                    binding.mainBottomNav.visibility = View.GONE
                }
            }
        }

        settings.registerOnSharedPreferenceChangeListener { sharedPrefs, key ->
            when(key) {
                "theme" -> {
                    val settingsTheme = sharedPrefs.getString("theme", "sys")

                    AppCompatDelegate.setDefaultNightMode(when (settingsTheme)
                    {
                        "sys" ->   AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
                        "light" -> AppCompatDelegate.MODE_NIGHT_NO
                        "dark" ->  AppCompatDelegate.MODE_NIGHT_YES
                        else ->    AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM
                    })

                    recreate()
                }
            }
        }
    }

    override fun onSupportNavigateUp(): Boolean {
        return navController.navigateUp(appBarConfig) || super.onSupportNavigateUp()
    }
}